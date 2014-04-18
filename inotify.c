#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>

static const char* s_cszEvtName[] = {
    "IN_ACCESS       ", // 0x00000001 - File was accessed
    "IN_MODIFY       ", // 0x00000002 - File was modified
    "IN_ATTRIB       ", // 0x00000004 - Metadata changed
    "IN_CLOSE_WRITE  ", // 0x00000008 - Writtable file was closed
    "IN_CLOSE_NOWRITE", // 0x00000010 - Unwrittable file closed.
    "IN_OPEN         ", // 0x00000020 - File was opened
    "IN_MOVED_FROM   ", // 0x00000040 - File was moved from X
    "IN_MOVED_TO     ", // 0x00000080 - File was moved to Y
    "IN_CREATE       ", // 0x00000100 - Subfile was created
    "IN_DELETE       ", // 0x00000200 - Subfile was deleted
    "IN_DELETE_SELF  ", // 0x00000400 - Self was deleted
    "IN_MOVE_SELF    "  // 0x00000800 - Self was moved
};
static const int s_ciNbOfEvtType = sizeof(s_cszEvtName)/sizeof(s_cszEvtName[0]);

struct MY_RESOURCES {
	int fd;			// Hold file descriptor from inotify_init
	int wd;			// Hold watch descriptor on selected folder from inotify_add_watch
};
static MY_RESOURCES s_MyRes = {-1, -1};

struct FOLDER_CTX {
    char*           szFolder;
    unsigned int    uMask;
};

/****************************************************************************\
    usage
    Quick help about parameters available.
\****************************************************************************/
void usage()
{
    printf("Usage:\n");
    printf("\t-? or -h --help. Show this help\n");
    printf("\t-d --directory. Select the directory to monitor. Default: current\n");
    printf("\t-e --events. Specify events to monitor. Default: IN_ALL_EVENTS\n");
    printf("Example monitor /dev for IN_CREATE | IN_DELETE events. Plug a USB device to see the Plug & Play effect.\n");
    printf("\t./inotify -d ""/dev"" -e 300\n");
}

/****************************************************************************\
    ParseCmdLine
    Using getopt_long to parse argument
    Setup FOLDER_CTX structure with user settings
\****************************************************************************/
bool ParseCmdLine(int argc, char* argv[], FOLDER_CTX& ctx)
{
    bool bStop = false;
    const char *short_options    = "?hd:e:";
    struct option long_options[] = {
        {"help"         , 0 , 0 , '?'} ,
        {"events"       , 1 , 0 , 'e'} ,   // event type to monitor
        {"directory"    , 1 , 0 , 'd'} ,   // select directory to monitor
        {0              , 0 , 0 , 0}       // mark end of options
    };

    int option_index = 0;
    int ch = 0;
    bool bExit = false;
    while((ch = getopt_long(argc,argv,short_options,long_options,&option_index))!=-1 && !bExit){
        switch(ch) {
        case '?':
        case 'h':
            usage();
            bStop = bExit = true;
            break;
        case 'd':
            ctx.szFolder = optarg;
            break;
        case 'e':
            ctx.uMask = strtoul( optarg, NULL, 16);
            break;
        default:
            printf("Try: '%s -?' for more information.\n", argv[0]);
            bStop = bExit = true;
            break;
        }
    }
    return bStop;
}

/****************************************************************************\
    ShowEvents
    Display in event names selected to watch over a folder
\****************************************************************************/
void ShowEvents(unsigned int uMask)
{
    for(int i=0; i<s_ciNbOfEvtType; i++) {
        if(uMask & 1) {
            fprintf(stderr, "%s\n", s_cszEvtName[i]);
        }
        uMask >>= 1;
    }
}

/****************************************************************************\
    ShowEvents
    Display in event names selected to watch over a folder
\****************************************************************************/
void PrintAction(unsigned int uMask, char* szAction)
{
    bool bEvt = false;
    const char* cszType = (uMask & IN_ISDIR) ? "dir " : "file";
    sprintf(szAction, "0x%08X\t%s\t", uMask, cszType);
    for(int i=0; i<s_ciNbOfEvtType; i++) {
        if(uMask & 1) {
            strcat(szAction, s_cszEvtName[i]);
            strcat(szAction, "\t");
            bEvt = true;
        }
        uMask >>= 1;
    }
    if(!bEvt) {
        strcat(szAction, "Invalid event: ");
    }
}


/****************************************************************************\
    ReadEvents
    Allow for 64 simultanious events
\****************************************************************************/
void ReadEvents(int fd, const char* szFolder)
{
    const unsigned int cuNbOfEvents = 64;
    const unsigned int cuBufSize = (sizeof(struct inotify_event)+FILENAME_MAX)*cuNbOfEvents;
    unsigned char byData[cuBufSize] = {0};

    ssize_t len = read (fd, byData, sizeof(byData));

    ssize_t i = 0;
    while (i < len) {
        struct inotify_event*   pEvt = (struct inotify_event*)&byData[i];
        // The max action string length is:
        // sizeof(s_cszEvtName[0]) + sizeof("file") + FILENAME_MAX + few spaces
        // For simplicity we make it twice the sizeof(s_cszEvtName[0])
        char action[2*sizeof(s_cszEvtName[0])+FILENAME_MAX] = {0};

        PrintAction(pEvt->mask, action);

        if (pEvt->len)
            strcat (action, pEvt->name);
        else
            strcat (action, szFolder);

        printf ("%s\n", action);
        i += sizeof(struct inotify_event) + pEvt->len;
    } while(false);
}

/****************************************************************************\
    ReleaseResources
\****************************************************************************/
void ReleaseResources(MY_RESOURCES& res)
{
    // Release resources.
    if(res.wd >= 0 && res.fd >= 0) {
        inotify_rm_watch(res.fd, res.wd);
		res.wd = -1;
	}

    if(res.fd >= 0) {
        close(res.fd);
		res.fd = -1;
	}
}

/****************************************************************************\
    SignalHandler
    Setup context information
    When terminate the user press Ctrl+C. We need to hook it
	cleanup resources before leaving
\****************************************************************************/
static void SignalHander(int sig, siginfo_t *siginfo, void *context)
{
    // First thing we set back the handler to the default one.
    // After the first execution of our handler we will not be called anymore
	// In any case we are exiting the program.
	// It is for completeness but not to useful here
    signal(sig, SIG_DFL);

	ReleaseResources(s_MyRes);

	// Clean up done. Time to exit program now
    fprintf (stderr, "Cleanup done. Time to exit.\n");
	exit(0);
}

/****************************************************************************\
    SignalHandler
		pass iSignal you like to hook. For Ctrl+C use SIGINT
    Setup context information
    When terminate the user press Ctrl+C. We need to hook it
	cleanup resources before leaving
\****************************************************************************/
int SetSignalHandler(int iSignal)
{
	int iRet = -1;
	do {
		fprintf(stderr, "Set signal handler on Ctrl+C\n");

		struct sigaction act = {0};
		iRet = sigemptyset( &act.sa_mask );
		if(iRet < 0) {
		    fprintf(stderr, "%d = sigemptyset(0x%p.sa_mask);\n", iRet, &act.sa_mask);
			break;
		}
		act.sa_sigaction = &SignalHander;
		act.sa_flags = SA_SIGINFO;

		// Hook singal SIGINT (Ctrl+C)
		iRet = sigaction(iSignal, &act, NULL);
		if (iRet < 0) {
		    fprintf(stderr, "%d = sigaction(%d, 0x%p, NULL);    \n", iRet, iSignal, &act);
			break;
		}

	} while(false);

	return iRet;
}

/****************************************************************************\
    main
    Setup context information
    Read and parse command line arguments
    Setup inotify to monitor selected folder with desired events
    Read and wait for event until Ctrl+C is pressed
\****************************************************************************/
int main (int argc, char *argv[])
{
    do {
	// Install handler on Ctrl+C to clean up resources.
	int iRet = SetSignalHandler(SIGINT);
	if(iRet < 0) {
		break;
	}

        FOLDER_CTX ctx = {
            (char*)".",     // current directory
            IN_ALL_EVENTS   // select all events
        };
        bool bRet = ParseCmdLine(argc, argv, ctx);
        if(bRet) {
            break;
        }
        fprintf (stderr, "Monitor directory %s for the following event(s) (0x%X)\n", ctx.szFolder, ctx.uMask);
        ShowEvents(ctx.uMask);

        s_MyRes.fd = inotify_init();
        if (s_MyRes.fd < 0) {
            fprintf (stderr, "%d =inotify_init(). Err: %s\n", s_MyRes.fd, strerror(errno));
            break;
        }

        s_MyRes.wd = inotify_add_watch (s_MyRes.fd, ctx.szFolder, ctx.uMask);
        if (s_MyRes.wd < 0) {
            fprintf (stderr, "%d = inotify_add_watch(%d, %s, %u). Err: %s\n",
                s_MyRes.wd, s_MyRes.fd, ctx.szFolder, ctx.uMask, strerror(errno));
            break;
        }

        // print description as first line.
        fprintf(stderr, "  MASK  \tTYPE\tEVENT NAME      \tTARGET\n");
        // For now press Ctrl+C to terminate program
        while (true) {
          ReadEvents(s_MyRes.fd, ctx.szFolder);
        }

    } while(false);

    return 0;
}
