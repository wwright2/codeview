// ***************************************************************************
//  ... Copyright (c) 2012 Luminator
//  ... All rights reserved.
//  ... Any use without the prior written consent of Luminator
//      is strictly prohibited.
// ***************************************************************************
// ***************************************************************************
//
//  Filename:       windowList.c
//
//  Description:    List management routines for windows
//
//  Revision History:
//  Date        Name            Ver     Remarks
//
//  Notes:
//
// ***************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>

#include "window.h"
#include "windowList.h"
#include "log.h"

/*****************************************************************************/

int addWindow (WINDOW **listref, WINDOW *w) {
    LOGDBG ("in addWindow (listref 0x%p, list 0x%p, window 0x%p)", 
	listref, *listref, w);

    WINDOW *list = *listref;

    if (list == NULL) {
        LOGTRACEW ("Adding first window in list", 0);
        *listref = w;
        w->prev = w->next = NULL;
        return 0;
    }

    seekListStart(list);

    WINDOW *last = NULL;
    do {
        if (list->priority > w->priority) {
            LOGTRACEW ("Inserting before higher priority node", 0);
            break;
        }
        last = list;
        list = list->next;
    } while (list);

    w->next = list;
    w->prev = last;

    if (last) last->next = w;
    else {
        LOGTRACEW ("Inserting at the begining of the list", 0);
        *listref = w;
    }

    if (list) list->prev = w;
    else {
        LOGTRACEW ("Appending to the end of the list", 0);
    }

    LOGTRACEW ("listref = %p", *listref);

    return 0;
}

/*****************************************************************************/

int removeWindow (WINDOW **listref, WINDOW *w) {
    if (!*listref) return 0;

    WINDOW *list = *listref;
    if (!list) return 0;

    seekListStart(list);

    LOGDBG ("Removing window #%d (%p)", w->id, w);

    if (w == list) {
        LOGTRACEW ("Removing window from head of list", 0);
        *listref = w->next;
        list = *listref;
    }

    if (w->prev) w->prev->next = w->next;
    if (w->next) w->next->prev = w->prev;

    w->next = w->prev = NULL;

    LOGDBG ("list = %p", list);

    return 0;
}

/*****************************************************************************/

WINDOW *findWindowById (WINDOW *list, int id) {
    if (!list) return NULL;

    /* rewind */
    seekListStart(list);

    /* seek to given id */
    while (list && (list->id != id)) list = list->next;

    if (list) {
        LOGDBG ("found window by id #%d,  (window %d, %p)", id, list->id, list);
    } else {
        LOGDBG ("NOTE: Did not find window matching id #%d", id);
    }
    /* return window (or NULL if not found */
    return list;
}

/*****************************************************************************/

void removeWindowId (WINDOW **listref, int id) {
    LOGDBG ("Entered removeWindowId  (id: %d)", id);

    WINDOW *win = findWindowById(*listref, id);
    if (!win) {
        LOGTRACEW ("Window not found when trying to remove", 0);
        return;
    }

    LOGTRACEW ("Removing window #%d (%p)", win->id, win);
    removeWindow(listref, win);
    return;

}

/*****************************************************************************
 * Look for a window with the given dimensions, which are x, y, height and
 * width packed into a 32-bit integer.
 */ WINDOW *getWinFromDim (WINDOW *list, uint32_t dim, int pri) {
    LOGDBG ("in getWinFromDim (list: %p, dim: 0x%X, pri: %d)", list, dim, pri);
    if (!list) return NULL;

    /* rewind */
    seekListStart(list);

    for (; list; list = list->next) {
        if ((list->userData == dim) && (list->priority == pri)) {
            LOGDBG ("found window #%d (%p) matching: (userData: 0x%X == 0x%X, priority:  %d == %d) ", list->id, list, list->userData, dim, list->priority, pri);
            return list;
        }
    }
    
    LOGDBG ("Did not find window", 0);
    return NULL;
}

/*****************************************************************************/

void clearWinList (WINDOW **listref) {
    WINDOW *list = *listref;
    if (!list) return;

    /* rewind */
    seekListStart(list);

    while (list) {
        WINDOW *next = list->next;
        delWindow(list);
        list = next;
    }

    *listref = NULL;

    return;
}

