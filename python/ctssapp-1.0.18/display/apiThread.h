#ifndef APITHREAD_H
#define APITHREAD_H

#include "ctaList.h"
#include "sharedData.h"

#define MAX_ARRIVALS 8
void *apiThreadLoop (void *);

//
// This keeps track of where fields should be and how wide they should
// be. It's only used by apiThread.c and arrivalFuncs.c
//
typedef struct
{
    int busCol;
    int busWidth;
    int busAlign;

    int rteCol;
    int rteWidth;
    int rteAlign;

    int dueCol;
    int dueWidth;
    int dueAlign;
} FIELD_DATA;

extern FIELD_DATA *fd;
extern CTA_LIST winTmp;
extern uint8_t tmpColor[];

static inline int colsNeeded (char c)
{
    if ((c < ctaConfig.f->startIndex) || 
        (c > ctaConfig.f->startIndex + ctaConfig.f->numGlyphs))
        return 0;

    if (ctaConfig.f->offsets[(int)c] == 0)
        return 0;

    return ctaConfig.f->glyphTable[(int)c].width;
}

WINDOW *mkWin(int row,int col,int width,int just,const char *txt,int line);
void doLongSign (char **lines, int numLines);
int  doShortSign (char **lines, int numLines);
void doNoDataMsg (char **lines, int numLines);
void doError (char **lines, int numLines);
int  addToStandbyList (WINDOW *w);
int  arrivalsInit ();
int  wordWrapNext (const char *s, int max, int *start, char *res);
void fillWinList  (int work);
//void emptyList (CTA_LIST *head);
void emptyTmpList ();
void emptyStandbyList (CTA_LIST *head);
void updateTmpWindowList ();
int  getLines (char *msg, char *line[MAX_ARRIVALS+1]);

#endif
