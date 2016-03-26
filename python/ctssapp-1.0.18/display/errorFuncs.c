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

extern CTA_LIST winTmp;
extern uint8_t tmpColor[3];

void doError (char **lines, int numLines)
{
    char buf[1024];
    int start = 0;
    int i = 0;
    int line = 1;
    int row = 0;
    WINDOW *w;

    for (i = 0; i < numLines; ++i)
    {
	int linelen = strlen (lines[i]);

	for (;;)
	{
	    if (start >= linelen)
		break;

	    wordWrapNext (lines[i], ctaConfig.dp->cols, &start, buf);
	    w = mkWin(row,0,dp->cols,ALIGN_LEFT,buf,line);
	    if (w == 0) continue;
	    addToStandbyList (w);
	    
	    line = line == 4 ? 1 : line + 1;
	    row += ctaConfig.f->height;
	    if (row >= 32) row = 0;
	}
    }
}

