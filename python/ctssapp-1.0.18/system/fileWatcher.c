#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <syslog.h>
#include <ctype.h>
#include <syslog.h>
#include <stdarg.h>
#include <regex.h>

static char events[sizeof (struct inotify_event) * 128];
static const char *pat = "ctssUpdatePkg-[0-9]+[.][0-9]+[.][0-9]+[.]tar[.]bz2";
static regex_t regex;
static regmatch_t match[3];
static char errBuf[1024];
static int priority = LOG_CONS | LOG_PID;


#if 0
// debug function
const char *eventToStr (int mask)
{
    int i;
    static char s[1024];
    s[0] = 0;
    
    for (i = 0; i < 12; ++i)
    {
        switch (mask & (1 << i))
        {
        case IN_ACCESS:
            strcat (s, "IN_ACCESS ");
            break;
        case IN_ATTRIB:
            strcat (s, "IN_ATTRIB ");
            break;
        case IN_CLOSE_WRITE:
            strcat (s, "IN_CLOSE_WRITE ");
            break;
        case IN_CLOSE_NOWRITE:
            strcat (s, "IN_CLOSE_NOWRITE ");
            break;
        case IN_CREATE:
            strcat (s, "IN_CREATE ");
            break;
        case IN_DELETE:
            strcat (s, "IN_DELETE ");
            break;
        case IN_DELETE_SELF:
            strcat (s, "IN_DELETE_SELF ");
            break;
        case IN_MODIFY:
            strcat (s, "IN_MODIFY ");
            break;
        case IN_MOVE_SELF:
            strcat (s, "IN_MOVE_SELF ");
            break;
        }
    }
    
    return s;
 }
#endif

void handleEvent (struct inotify_event *e)
{
    int ret;
    const char *name = e->name;
    static char cmd[1024];

    if (! e->mask & IN_CLOSE_WRITE)
        return;

    ret = regexec (&regex, name, sizeof match / sizeof match[0], match, 0);
    if (ret != REG_NOMATCH)
    {
        syslog (priority, "matched %s\n", name);

        // 
        // Run the /usr/bin/doUpdate.sh script
        //

        sprintf (cmd, "/usr/bin/doUpdate.sh /home/cta/%s", name);
        system (cmd);
    }
}
      

int regexInit ()
{
    int ret;

    if ((ret = regcomp (&regex, pat, REG_EXTENDED)) != 0)
    {
        regerror (ret, &regex, errBuf, sizeof errBuf);
        syslog (priority, errBuf);
        fprintf (stderr, "%s\n", errBuf);
        return -1;
    }

    return 0;
}

int main (int argc, char *argv[])
{
    int fd;
    int watch;
    int ret = 0;
    int flags = IN_CLOSE_WRITE;
    struct stat sb;
    const char *dir;

    openlog (argv[0], LOG_CONS | LOG_PID, LOG_DAEMON);

    printf ("1\n");
    if (argc == 1)
        dir = "/home/cta";
    else
        dir = argv[1];

    printf ("2\n");
    if (stat (dir, &sb) == -1)
    {
        syslog (priority, 
            "Could not stat %s (must be a directory): %s\n", strerror (errno));
        exit (1);
    }

    printf ("3\n");
    if (! S_ISDIR(sb.st_mode))
    {
        syslog (priority,  "%s is not a directory\n", dir);
        exit (1);
    }

    printf ("4\n");
    if (argc == 3)
        pat = argv[2];

    printf ("5\n");
    if (regexInit () == -1)
        exit (1);

    printf ("6\n");
    if ((fd = inotify_init ()) == -1)
    {
        syslog (priority,  "Error in inotify_init: %s\n",strerror (errno));
        exit (1);

    }

    printf ("7\n");
    if ((watch = inotify_add_watch (fd, dir, flags)) == -1)
    {
        syslog (priority, "Error in inotify_add_watch: %s\n",strerror (errno));
        exit (1);
    }

    for (;;)
    {
        int byte;

        memset (events, 0, sizeof events);
        ret = read (fd, events, sizeof events);
        if (ret == -1)
        {
            syslog (priority,  "read returned %s\n",strerror (errno));
            continue;
        }

        //printf ("\n\nread returned %d\n", ret);

        //
        // The size of an inotify event is not necessarily == sizeof
        // (inotify_event) because the structure has an array as its last
        // elemtnt, and that array may extend the size of the struct. 
        //
        // To tell when we've gotten to the end of the items returned by read,
        // we have to add up the size of the struct, plus additional bytes
        // allocated for the file name.
        //
        // The size of the struct is 16 on the CTSS system, and each one has at
        // least 16 bytes for the name. If additional bytes are needed for the
        // name they appear to be allocated in chunks of 16 bytes. If one byte
        // beyond 32 is needed (for a null maybe), another 16 bytes is
        // allocated, so the total size used for that entry would be 48.
        //

        byte = 0;
        while (byte < ret)
        {
            struct inotify_event *ev = 
                (struct inotify_event*)&events[byte];

            if (ev->mask == 0)
                break;

            //printf ("file = %s, event = %s\n", 
            //    ev->name, eventToStr (ev->mask));

            handleEvent (ev);
            byte += sizeof (struct inotify_event) + ev->len;
        }
    }
}
