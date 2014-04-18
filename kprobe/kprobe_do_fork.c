/*
 * Monitor process creation and termination 
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/pid_namespace.h>
#include <linux/kprobes.h>
#include <linux/proc_fs.h>

#define ERROR_LEVEL     (1)
#define WARNING_LEVEL   (2)
#define INFO_LEVEL      (3)
#define DETAIL_LEVEL    (4)
#define trace(level, fmt, ...) if(level <= iDebugLevel) printk (fmt, ## __VA_ARGS__)

#define MAX_LINE_SIZE   (150)   // Max line size in [char]
#define MAX_NB_OF_LINES (50)    // Max number of line in the log buffer.

/*
   Description of parameters that can be passed to the module on load
*/
static int iDebugLevel = 3;
module_param(iDebugLevel,int,0444);
MODULE_PARM_DESC(iDebugLevel, "Select debug level. Default is 1 (error)");

static int iMaxLineSize = MAX_LINE_SIZE;
module_param(iMaxLineSize,int,0444);
MODULE_PARM_DESC(iMaxLineSize, "Max string size per line. Default is 150");

static int iMaxNbOfLines = MAX_NB_OF_LINES;
module_param(iMaxNbOfLines,int,0444);
MODULE_PARM_DESC(iMaxNbOfLines, "Max # of line in log. Default is 50");

#define PROC_NAME   "taskinfo"
static struct proc_dir_entry*    proc_taskinfo = NULL;
struct proc_context {
    bool bNewRead;      // flag to notify a new read from the proc file
    int  rdOff;         // current reading offset from the log
    int  iRemLogSize;   // Remaining data from the last round.
};
static struct proc_context s_ProcCtx;

// Log buffer and state variables.
static char*    s_pLog = NULL;      // pointer to the log buffer
static atomic_t s_wrOff;            // current write offset to the log buffer
static bool     s_bWrapped = false; // Indicate if we already looped through the buffer
static atomic_t s_cnt;              // counter entry to the log

//=========================== Begin Proc file section ===================================
/*
   ReadProcFile
*/
int ReadProcFile(char* buf, char** start, off_t offset, int len, int* eof, void* context)
{
    // Keep some room between the current write location and the read. 
    // This is to avoid getting data that maybe written while we place them on the proc file.
    const int ciNbSafetyLines = 4;

    struct proc_context* ctx = (struct proc_context*)context;
    // Convert the proc info into a string to the user space.
    int iRet = 0;
    int iDataAvail = (s_bWrapped) ? 
        (iMaxNbOfLines-ciNbSafetyLines) * iMaxLineSize : atomic_read(&s_wrOff);

    // Compute data available to expose to proc file taskinfo.
    if (ctx->bNewRead) {
        int wrOff = atomic_read(&s_wrOff);
        ctx->bNewRead = false;
        ctx->rdOff = 0;
        ctx->iRemLogSize = 0;
        // compute the data size available to be read
        if(s_bWrapped) {
            // If a wrap around already happen, read valid data at the end of the last 
            // wrap. From wrOff + 2 to the end. The +2 it to avoid race condition if 
            // new data are coming in as we read the old one.
            if((wrOff + (ciNbSafetyLines*iMaxLineSize)) < (iMaxNbOfLines * iMaxLineSize)) {
                ctx->rdOff = wrOff + ciNbSafetyLines * iMaxLineSize;
                ctx->iRemLogSize = (iMaxNbOfLines * iMaxLineSize) - ctx->rdOff;
            } else {
                ctx->rdOff = wrOff - iDataAvail;
            }
        }
        trace(DETAIL_LEVEL, "Entry: wrOff=%d, rdOff=%d, offset=%lu, iRemLogSize=%d, iDataAvail=%d, s_bWrapped=%d\n", 
            wrOff, ctx->rdOff, offset, ctx->iRemLogSize, iDataAvail, s_bWrapped);
    }

    // Copy data available in the log buffer to the proc file.
    if(offset < ctx->iRemLogSize) {
        // Read available in the last round
        iRet = min(ctx->iRemLogSize-(int)offset, len);
        memcpy(buf, &s_pLog[ctx->rdOff+offset], iRet);
        trace(DETAIL_LEVEL, "SEC.#1: offset=%lu, ctx->rdOff=%d, iRet=%d, s_iRemLogSize=%d\n", 
            offset, ctx->rdOff, iRet, ctx->iRemLogSize);
    } else {
        // Data available in the same round
        if(offset < iDataAvail) {
            // Read data from the last round
            iRet = min(iDataAvail-(int)offset, len);
            memcpy(buf, &s_pLog[offset-ctx->iRemLogSize], iRet);
            trace(DETAIL_LEVEL, "SEC.#2: offset=%lu, ctx->rdOff=%d, iRet=%d, s_iRemLogSize=%d\n", 
                offset, ctx->rdOff, iRet, ctx->iRemLogSize);
        }
    }
    *start = buf;

    // When done with the full proc read, reset the flag for the next time.
    if (iRet == 0) {
        ctx->bNewRead = true;
    }
    trace(DETAIL_LEVEL, "%d = ReadProcFile(0x%p, 0x%p, %ld, %d, 0x%p, 0x%p), s_bNewRead=%d, iDataAvail=%d, ctx->rdOff=%d\n", 
        iRet, buf, start, offset, len, eof, ctx, ctx->bNewRead, iDataAvail, ctx->rdOff);

    return iRet;
}

/*
 CreateProcFile
*/
int CreateProcFile(void)
{
    proc_taskinfo = create_proc_entry(PROC_NAME, 0, NULL);
    if(proc_taskinfo) {
        proc_taskinfo->read_proc  = ReadProcFile;
        proc_taskinfo->data       = (void*)&s_ProcCtx;
    }

    return (proc_taskinfo) ? 0 : -EFAULT;
}

/*
    ReleaseProcFile
*/
void ReleaseProcFile(void)
{
    // Remove proc file
    if(proc_taskinfo) {
        remove_proc_entry(PROC_NAME, 0);
    }
}
//=========================== End Proc file section ==================================

/*
 Find the task related to a specific PID
*/
struct task_struct* find_task_from_pid(long pid)
{
    struct task_struct *task = NULL;
    for_each_process(task) {
        //     trace(DETAIL_LEVEL, "%s[%d]\n", task->comm, task->pid);
        if(task->pid == pid) {
            trace(INFO_LEVEL, "For PID (%d) task is %s...", task->pid, task->comm);
            break;
        }
    }
    return task;
}

/*
    Record task information into a circular buffer
    The circular buffer has a fixed size of iMaxNbOfLines * iMaxLineSize that can be 
    configured with parameters on the command line when loading the module.
    The strings inside a line can be any length between 1 to iMaxLineSize-1 to guaranty 
    null termination

    Below is a dummy example of the string formatting inside the circular buffer.

     -------------------- iMaxLineSize ---------------------
   /                                                         \
  +-----------------------------------------------------------+
  |This is the first line in the log                          | \
  |Each line has a fix size of iMaxLineSize. Default is 150   |  \
  |Each line are zeroed out before placing the new data in    |   \
  |It insures that the string is always null terminated       |    \
  |...........................................................|     +-> iMaxNbOfLines
  |The number of line in the buffer is iMaxNbOfLines          |    /
  |Number of line can be conifgured when loading the module   |   /
  |This is the last line in the log                           |  /
  |The default max line value is 50 lines                     | /
  +-----------------------------------------------------------+

*/
void RecordTaskInfo(struct task_struct * task, char* szTitle)
{
    int iRet = 0;
    int iNewOff = 0;
    int iOff = atomic_read(&s_wrOff);
    int iCnt = atomic_inc_return(&s_cnt);

    if(task == NULL) {
        task = current;
        trace(WARNING_LEVEL, "SHOULD NEVER HAPPEN. But if it does provide a valid task pointer to avoid crash, task=0x%p\n", task);
    }
  
    // Reset write offset to the beginning of the circular buffer when reached the end
    if(iOff >= (iMaxNbOfLines * iMaxLineSize)) {
        s_bWrapped = true;
        atomic_set(&s_wrOff, 0);
        iOff = 0;
    }
    // Clear old content before we write new data. Make clean end of string until the 
    // next one in the log buffer
    memset(&s_pLog[iOff], 0, iMaxLineSize);

    // Log new trace in the circular buffer
    iRet = sprintf(&s_pLog[iOff], "[%d] %s, pid=%d, tgid=%d, parent=%d, grplead=%d,  nvcsw=%lu, nivcsw=%lu, name=%s\n", 
        iCnt, szTitle, task->pid, task->tgid, task->real_parent->pid, task->group_leader->pid, task->nvcsw, task->nivcsw, task->comm);
    if(iRet > iMaxLineSize) {
        trace(ERROR_LEVEL, "iRet=%d, Max size=%d\n", iRet, iMaxLineSize);
    }
    iNewOff = atomic_add_return(iMaxLineSize, &s_wrOff);

    trace(DETAIL_LEVEL, "iRet=%d, off=%d, newOff=%d....%s", iRet, iOff, iNewOff, &s_pLog[iOff]);
}

//=============================== Begin do_fork handlers on exit ========================

/* 
    Handler for jprobe or kretprobe when an exception occurs.
    Right now we only trace an error and don't handle the fault. Trace and dump stack
    So it returns 0 and the kernel will do its best to hanlde it.
*/
int probe_handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr)
{
    trace(ERROR_LEVEL, "fault_handler: p->addr=0x%p, trap #%dn", p->addr, trapnr);

    dump_stack();   // will add a trace in the system log of the current stack.

    // Return 0 because we don't handle the fault.
    return 0;
}

/* 
    Execute when do_fork returns. The return value is the newly created PID. 
    The PID can be accessed in the ax register. We use the PID to find the 
    related task before we log data in our circular buffer
*/
static int ret_do_fork(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    // Didn't find a way to find the task corresponding to a pid. 
    // Use helper function to walk task list
    struct task_struct *task = find_task_from_pid(regs->ax);

    RecordTaskInfo(task, "do_fork(....)");

    return 0;
}

static struct kretprobe kret_do_fork = {
    .handler        = ret_do_fork,
    .kp = {
        .symbol_name    = "do_fork",
        .fault_handler  = probe_handler_fault,
    },
    // Probe up to 1 instance concurrently.
    .maxactive      = 1,
};
//=============================== End do_fork handlers on exit ==========================


//=============================== Begin free_task handlers on entry =====================
/* 
    Execute when free_task is called. the only parameter passed is the task_struct being 
    terminated. Pass the task to be recorded in the circular buffer.
*/
static void jfree_task(struct task_struct *task)
{
    RecordTaskInfo(task, "free_task(..)");
 
    // Always end with a call to jprobe_return().
    jprobe_return();
    // return 0;
}

static struct jprobe jprobe_free_task = {
    .entry          = jfree_task,
    .kp = {
        .symbol_name    = "free_task",
        .fault_handler  = probe_handler_fault,
    },
};
//========================= End free_task handlers on entry =============================
 

/*
    Helper cleanup resources routine.
    Release resources in the reverse order we acquired them
*/
void ReleaseResources(bool bFreeTsk, bool bRetDoFork)
{
    ReleaseProcFile();

    if(bFreeTsk) {
        unregister_jprobe(&jprobe_free_task);
        trace(DETAIL_LEVEL, "unregister_jprobe(0x%p)\n", &jprobe_free_task);
    }
    if(bRetDoFork) {
        unregister_kretprobe(&kret_do_fork);
        trace(DETAIL_LEVEL, "unregister_kretprobe(0x%p)\n", &kret_do_fork);
    }

    if(s_pLog) {
        kfree(s_pLog);
    }
}
 
/*
    Module entry point 
    Acquire resources and initialize global variables.
    Resources allocated are:
    1. Allocate circular log buffer
    2. Install hooks on
    2.1 On entry call of free_task
    2.2 On exit call of do_fork
    3. Create a proc file 
*/ 
int probe_processes_init(void)
{
    int iRet              = 0;
    int ijprobe_free_task = -1;
    int iret_do_fork      = -1;
    int iproc_file        = -1;
    trace(DETAIL_LEVEL, "Entering, init_module(); iMaxLineSize=%d, iMaxNbOfLines=%d, iDebugLevel=%d\n", 
        iMaxLineSize, iMaxNbOfLines, iDebugLevel);

    atomic_set(&s_wrOff, 0);
    atomic_set(&s_cnt, 0);

    do {
        // Create circular log buffer
        s_pLog = (char*)kmalloc(iMaxLineSize * iMaxNbOfLines, GFP_KERNEL);
        if(s_pLog == NULL) {
            iRet = -ENOMEM;
            break;
        }
        memset(s_pLog, 0, iMaxLineSize * iMaxNbOfLines);

        // Register hook on the entry of free_task kernel method
        if ((ijprobe_free_task = register_jprobe(&jprobe_free_task)) < 0) {
            trace(ERROR_LEVEL, "register_jprobe(%s) failed, returned %d\n", 
                jprobe_free_task.kp.symbol_name, ijprobe_free_task);
            iRet = ijprobe_free_task;
            break;
        }
        trace(DETAIL_LEVEL, "On call of %s at %p, handler addr %p\n", 
            jprobe_free_task.kp.symbol_name, jprobe_free_task.kp.addr, jprobe_free_task.entry);

        // Register hook on the exit of do_fork kernel method
        if ((iret_do_fork = register_kretprobe(&kret_do_fork)) < 0) {
            trace(ERROR_LEVEL, "register_kretprobe(%s) failed, returned %d\n", 
                kret_do_fork.kp.symbol_name, iret_do_fork);
            iRet = iret_do_fork;
            break;
        }
        trace(DETAIL_LEVEL, "On return of %s at %p, handler addr %p\n", 
            kret_do_fork.kp.symbol_name, kret_do_fork.kp.addr, kret_do_fork.handler);

        // Create a /proc/taskinfo file
        if((iproc_file = CreateProcFile()) < 0) {
            iRet = iproc_file;
            break;
        }
    } while(false);

    // Run clean up code if we fail to acquire any resources.
    if(iRet < 0) {
        ReleaseResources(ijprobe_free_task==0?true:false, iret_do_fork==0?true:false);
    }

    trace(INFO_LEVEL, "Exiting, %d = init_module();\n", iRet);
    return iRet;
}
 
/*
    Module exit point
    Release all resource acquired during module init
*/ 
void probe_processes_exit(void)
{
    trace(DETAIL_LEVEL, "Entering cleanup_module()\n");

    ReleaseResources(true, true);

    trace(INFO_LEVEL, "Exiting cleanup_module()\n");
}

module_init(probe_processes_init);
module_exit(probe_processes_exit);
 
MODULE_AUTHOR("Richard Nicolet");
MODULE_LICENSE("GPL");  // Kernel isn't tainted 
MODULE_DESCRIPTION("Monitor process creation and termination. Expose a proc file (taskinfo) to monitor the most recent activities\n"
"The log buffer can be control with parameters (iMaxLineSize) & (iMaxNbOfLines) on module load.\n\n"
"iMaxLineSize\n"
"+----------+\\ \n"
"|abcd......| \\ \n"
"|efghijk...|  +=> iMaxNbOfLines\n"
"|vwxyz.....| /\n"
"+----------+/\n");
