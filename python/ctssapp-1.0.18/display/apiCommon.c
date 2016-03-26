#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "window.h"
#include "log.h"
#include "sharedData.h"
#include "apiThread.h"

// ---------------------------------------------------------------------------
// copy up to max columns into res from s. The start parameter is
// modified to return the position after the terminating null.
//
int wordWrapNext (const char *s, int max, int *start, char *res)
{
    int tot   = 0;
    int cols  = 0;
    int tmp   = 0;
    int space = 0;
    int slash = 0; // also includes '-'
    int rpos  = 0;
    int i;
    int ret = 0;

    *res = 0;

    for (i = *start, rpos=0; i < strlen (s); ++i, ++rpos)
    {
        if ((! s[i]) || (s[i] == '\n'))
        {
            res[rpos] = 0;
	    ret = -1;
            break;
        }

	tmp = cols;
        cols += colsNeeded (s[i]);
        if (cols > max)
        {
	    if ((s[i] == '/') || (s[i] == '-'))
	    {
		// Break here, but save the slash for next line
		res[rpos] = 0;
		*start = i;
	    }

	    else if (s[i] == ' ')
	    {
		// same as slash but throw away the space
		res[rpos] = 0;
		space = 0;
		*start = i + 1;
	    }

	    else if ((slash) && (slash > space))
	    {
		res[slash+1] = 0;
		i -= (rpos - slash - 1); // back up to slash + 1
		*start = i;
	    }
 
            else if (space)
            {
                res[space] = 0;
		i -= (rpos - space - 1);
                *start = i;
            }

           else
            {
                res[rpos] = 0;
                *start = i + 1;
            }

	    return 0;
        }

        else
        {
            if (s[i] == ' ') space = rpos;
	    else if ((s[i] == '/') || (s[i] == '-')) slash = rpos;

            tot += cols;
            res[rpos] = s[i];
        }
    }

    *start = i;
    res[rpos] = 0;
    return ret;
}

// ----------------------------------------------------------------------------
//
int addToStandbyList (WINDOW *w)
{
    // Add the window to the standby list
    CTA_LIST *tmp = calloc (1, sizeof (CTA_LIST));
    if (tmp == 0)
    {
        LOGERR ("Could not allocate tmp %s %d", __FILE__, __LINE__);
        return -1;
    }
    
    tmp->data = w;
    ctalistAddBack (&winTmp, tmp);
    return 0;
}

// ----------------------------------------------------------------------------
//
WINDOW *mkWin(int row, int col, int width, int just, const char *txt, int line)
{
    LOGTRACE ("mkWin: making window for [%s], line = %d", txt, line);
    WINDOW *w = newWindow (
        dp,
        row,               // base row
        col,               // base column
        width,             // length of window in columns
        8,                 // height of window in rows
        0,                 // graphic offset
        0,                 // graphic offset
        TEXT,              // text or graphic
        3,                 // color depth - 24 bit supported initially
        tmpColor,          // color to use
        ctaConfig.f,       // default font
        just,              // alignment
        0,                 // effect
        0,                 // scroll
        txt,               // data to display
        strlen (txt),      // length of data
        0,                 // ms
        1,                 // id of window.
        1,                 // determines where window is in order
        -1);               // time to live in seconds, or -1
    
    if (w == 0)
    {
        LOGERR ("Could not create window: %s\n", winLastErrMsg ());
        return 0;
    }

    w->userData = line;
    return w;
}

// ----------------------------------------------------------------------------
//
void fillWinList  (int work)
{
    // caller must lock windowList
    while (! ctalistIsEmpty (&winTmp))
    {
        CTA_LIST *tmp = winTmp.next;
        ctalistDelEntry (winTmp.next);
        ctalistAddBack (windowList.lst[work], tmp);
    }
}

// ----------------------------------------------------------------------------
//
void emptyStandbyList (CTA_LIST *head)
{
    // caller must do any locking needed
    while (! ctalistIsEmpty (head))
    {
        CTA_LIST *tmp = head->next;

        ctalistDelEntry (tmp);
	if (tmp->data != 0) delWindow ((WINDOW*)tmp->data);
	memset (tmp, 0, sizeof *tmp);
	free (tmp);
    }
}

// ----------------------------------------------------------------------------
//
void emptyTmpList ()
{
    CTA_LIST *head = &winTmp;
    while (! ctalistIsEmpty (head))
    {
        CTA_LIST *tmp = head->next;

        ctalistDelEntry (tmp);
	if (tmp->data != 0) delWindow ((WINDOW*)tmp->data);
	memset (tmp, 0, sizeof *tmp);
	free (tmp);
    }
}

// ---------------------------------------------------------------------------
//
void updateTmpWindowList ()
{
    //
    // Once the message has been parsed and the pieces stored in windows,
    // replace the current standby list with the tmp list. This is a two step
    // ooperation because the render thread does the switch between active and
    // inactive lists, and we don't know when he'll do that. Once we get the
    // lock on the frame buffer, move these to the inactive list, and set the
    // newData member to 1.
    //

    int ptErr = 0;

    LOCK (windowList.lock, ptErr);
    if (ptErr) die ("Error locking mutex");

    int work = windowList.active ^ 1; // get the inactive list
    emptyStandbyList (windowList.lst[work]);
    fillWinList  (work);
    windowList.color[0] = tmpColor[0];
    windowList.newData = 1;

    UNLOCK (windowList.lock, ptErr);
    if (ptErr) die ("Error unlocking mutex");
}


// ---------------------------------------------------------------------------
//
int getLines (char *msg, char *line[MAX_ARRIVALS+1])
{
    int i = 0;
    line[i] = msg;
    
    memset (line, 0, sizeof (char *) * MAX_ARRIVALS+1);
    for (i = 0; i < MAX_ARRIVALS; ++i)
    {
	line[i] = msg;

        while (*msg && *msg != '\n')
            ++msg;
        
        if (*msg == '\n')
            *msg++ = 0;

        else 
	{
	    *msg = 0;
	    break;
	}
    }

    return i;
}

