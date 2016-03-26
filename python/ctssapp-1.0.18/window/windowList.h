#ifndef __WINDOWLIST_H__
#define __WINDOWLIST_H__

#include "window.h"

#define seekListStart(w) while (w->prev) w = w->prev
//#define seekListStart(w) do { __debug_lists("Seeking to list start of list %p", w); while (w->prev) { __debug_lists("Win %p, Prev = %p", w, w->prev); w = w->prev; } __debug_lists("Done seeking, start == %p", w); } while(0)
#define seekListEnd(w) while (w->next) w = w->next


int addWindow (WINDOW **listref, WINDOW *w);
int removeWindow (WINDOW **listref, WINDOW *w);
WINDOW *findWindowById (WINDOW *list, int id);
void removeWindowId (WINDOW **listref, int id);
WINDOW *getWinFromDim (WINDOW *list, uint32_t dim, int pri);
void clearWinList (WINDOW **listref);


#endif

