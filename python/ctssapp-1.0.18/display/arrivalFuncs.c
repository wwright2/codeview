#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <regex.h>

#include "log.h"
#include "ctaList.h"
#include "apiThread.h"
#include "sharedData.h"
#include "trackerconf.h"
#include "window.h"

static const char *regex = "^[ ]*(.{1,4})[ ]+(.+)[ ]+(.{1,4})$";
static regex_t re;
static regmatch_t match[5];
static char err[1024];
static int arrivalsInitDone = 0;
static const int matchSize = sizeof match / sizeof match[0];

extern int numWinTmp;
extern uint8_t tmpColor[3];
extern CTA_LIST winTmp;

static const int row1 = 4;
static const int row2 = 20;

void dumpWin (WINDOW *w);

// ---------------------------------------------------------------------------
//
void dumpWinList (CTA_LIST *head)
{

    printf ("\n=======\ndumpWinList:\n");
    if (ctalistIsEmpty (head))
    {
	printf ("list empty\n");
	return;
    }

    CTA_LIST *tmp = head->next;
    while (tmp != head)
    {
	printf ("line %d: %s\n", 
	    ((WINDOW*)tmp->data)->userData,
	    winGetText((WINDOW*)tmp->data));
	tmp = tmp->next;
    }
}

// ---------------------------------------------------------------------------
// Return 1 if the copy was successful and 0 if not.
//
static int copyMatch (const char *s, const regmatch_t *m, char *buf)
{
    int start = m->rm_so;
    int end   = m->rm_eo;
    int len   = end - start;
    int ret   = 1;

    *buf = 0;
    if (start == -1 || end == -1)
    {
        buf[0] = 0;
        ret = 0;
    }

    else
    {
        if (snprintf (buf, len + 1, "%s", s + start) <= 0)
        {
            LOGERR ("Error copying regex match: %s", strerror (errno));
            ret = 0;
        }
    }

    return ret;
}

// ---------------------------------------------------------------------------
//
static int splitArrival (const char *buf, char *bus, char *rte, char *time)
{
    int ret;

    *bus = 0;
    *rte = 0;
    *time = 0;

    LOGTRACE ("=======> splitArrival: string = [%s]", buf);
    memset (match, -1, sizeof match);
    ret = regexec (&re, buf, sizeof match / sizeof match[0], &match[0], 0);
    if (ret != 0)
    {
        regerror (ret, &re, err, sizeof err);
        LOGERR ("Error executing regex for [%s]: [%s]", buf, err);
        return -1;
    }

    if (copyMatch (buf, &match[1], bus) == 0)
	return -1;
    if (copyMatch (buf, &match[2], rte) == 0)
	return -1;
    if (copyMatch (buf, &match[3], time) == 0)
	return -1;

    return 0;
}

// ---------------------------------------------------------------------------
//
int splitShortArrival (
    const char *line, 
    char *bus, 
    char *rte1, 
    char *time,
    char *rte2)
{
    int max = fd->rteWidth;
    int start = 0;
    char rte[strlen (line) + 1];

    *bus = 0;
    *rte1 = 0;
    *rte2 = 0;
    *time = 0;

    if (splitArrival (line, bus, rte, time) != 0)
        return -1;

    wordWrapNext (rte, max, &start, rte1);
    wordWrapNext (rte, max, &start, rte2);

    return 0;
}

// ----------------------------------------------------------------------------
// These are always displayed four lines per page. If a rte is too
// long for its field, it's truncated.
//
void doLongSign (char **lines, int numLines)
{
    char arrival[3][60];
    int line = 1;
    int count = 0;
    int row  = 0;
    WINDOW *w;

    for (int i = 0; i < numLines; ++i)
    {
	if ((lines[i] == 0) || (strlen (lines[i]) == 0))
		continue;

        if (splitArrival (lines[i], arrival[0], arrival[1], arrival[2]) == 0)
        {
            char *txt = arrival[0];
            w = mkWin(row,fd->busCol,fd->busWidth,fd->busAlign,txt,line);
            if (w == 0)
                continue;
            addToStandbyList (w);

            txt = arrival[1];
            w = mkWin(row,fd->rteCol,fd->rteWidth,fd->rteAlign,txt,line);
            if (w == 0)
                continue;
            addToStandbyList (w);

            txt = arrival[2];
            w = mkWin(row,fd->dueCol,fd->dueWidth,fd->dueAlign,txt,line);
            if (w == 0)
                continue;
            addToStandbyList (w);
        }

        line = line == 4 ? 1 : line + 1;
        count += 1;
        row += ctaConfig.f->height;
	if (row >= 32) row = 0;
    }

    if (count == 1)
    {
        LOGTRACE ("Making dummy line, only received one");
        w = mkWin (16, 0, 1, 0, " ", 2);
        if (w != 0)
            addToStandbyList (w);
        else
            LOGFATAL ("Out of memory in %f %s", __FILE__, __LINE__);
    }
}

static void addEntry (
    char *bus,
    char *rte,
    char *due,
    char *rte2,
    int row,
    int line)
{
    WINDOW *w;
    int numLines = (*rte2 == 0) ? 1 : 2;
    int r1 = (numLines == 1) ? row : row - 4;
    int r2 = (numLines == 1) ? row : row + 4;


    w = mkWin(row, fd->busCol, fd->busWidth, fd->busAlign, bus, line);
    if (w == 0) return;
    addToStandbyList (w);

    w = mkWin(r1,fd->rteCol,fd->rteWidth,fd->rteAlign,rte,line);
    if (w == 0) return;
    addToStandbyList (w);
    
    w = mkWin(row,fd->dueCol,fd->dueWidth,fd->dueAlign,due,line);
    if (w == 0) return;
    addToStandbyList (w);
    
    if (numLines == 2)
    {
        w=mkWin(r2,fd->rteCol,fd->rteWidth,fd->rteAlign,rte2,line);
        if (w == 0) return;
        addToStandbyList (w);
    }
}

// ----------------------------------------------------------------------------
//
int doShortSign (char **lines, int numLines)
{
    char arr[4][60];
    int curLine = 1;
    int count = 0;

    emptyTmpList ();
    for (int i = 0; i < numLines; ++i)
    {
	if ((lines[i] == 0) || (strlen (lines[i]) == 0))
		continue;

        if (splitShortArrival (lines[i], arr[0], arr[1], arr[2], arr[3]) == 0)
        {
            switch (curLine)
            {
            case 1:
                addEntry (arr[0],arr[1],arr[2],arr[3],row1,curLine);
                curLine = 2;
                count += 1;
                break;
                
            case 2:
                addEntry (arr[0], arr[1], arr[2], arr[3], row2, curLine);
                curLine = 1;
                count += 1;
                break;

            default:
                LOGERR ("Invalid line in doShortSign: %d", curLine);
                break;
            }
        }

	else
	{
	    LOGERR ("An error was returned from splitShortArrival\n");
	    break;
	}

    }

    if (count == 1)
    {
        LOGTRACE ("Making dummy line, only received one");
        WINDOW *w = mkWin (16, 0, 1, 0, " ", 2);
        if (w != 0)
            addToStandbyList (w);
        else
            LOGFATAL ("Out of memory in %f %s", __FILE__, __LINE__);
    }

    //
    // Either an error occurred, or we're done. We need to be sure there's
    // an entry for line 2 in the list, otherwise it can happen that two
    // sets of windows get rendered on line 1.
    //
    // The test value isn't a typo. If the last line we processed was 1, then
    // curLine is now 2.
    //
    
    if (curLine == 2)
    {
	addEntry ("", "", "", "", (curLine == 1) ? row1 : row2, 2);
    }
	    
    return 0;
}

// ---------------------------------------------------------------------------
// Compile the regular expression here so we don't have to do it every time.
//
int arrivalsInit ()
{
    int ret;

    memset (&re, 0, sizeof re);
    memset (&match, 0, sizeof match);
    if ((ret = regcomp (&re, regex, REG_EXTENDED | REG_NEWLINE)) < 0)
    {
        regerror (ret, &re, err, 1024);
        return -1;
    }

    arrivalsInitDone = 1;
    return 0;
}

