//


#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <strings.h>
#include <string.h>
#include <getopt.h>

#include <time.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

#include <syslog.h>

#include <unistd.h>
#include <errno.h>

#include <ctssTimer.h>
#include <radlist.h>

// HIDDEN
struct timerlist
{
	RADLIST_ID			list;
	pthread_attr_t		attr;
	pthread_t 			thread;
	int					uSeconds;
	sem_t               semCount;

};

/* int timerProc(void	*tp)
 *
 * - Thread, every N microSeconds wake up and decrement Timers.
 * - This is not an accurate timer and degrades with the number of timers added to the list
 * - and execution time for t->callback() adds time till next call to the next call.
 * -  should wake up and check system ticks, to adjust for extra time.
 *
 */
void  *timerProc(void	*tp)
{
	struct timerlist * tl = (struct timerlist *) tp;

	Timer_id *timer, *rm ;

	while(1)
	{
		usleep(tl->uSeconds); //milliSec = 1000* microsec
		// Process list
		for (timer = (Timer_id *) radListGetFirst(tl->list) ;
			timer!=NULL;
			timer=(Timer_id *)radListGetNext(tl->list, (NODE_PTR)timer))
		{
			if (timer->exit)
			{
				// Marked for deletion.
				rm=timer;
				timer = (Timer_id*) radListGetNext(tl->list,(NODE_PTR ) timer);
				radListRemove(tl->list, (NODE_PTR ) rm);
				free (rm);
			}

			else if (timer->countdown > 0)
			{
				// if running
				if (timer->stopped == 0)
				{
					sem_wait(&tl->semCount);
						timer->countdown--;
					sem_post(&tl->semCount);

					if (timer->countdown == 0)
						timer->callback(timer->procwork);
				}
			}
		}
	}
}
// END HIDDEN



RADLIST_ID	timerCreateList(int uSecondsTick)
{
	struct timerlist 	* tlist;

	tlist = (struct timerlist *)malloc (sizeof (struct timerlist));
	tlist->list = radListCreate();
    tlist->uSeconds = uSecondsTick;

    sem_init(&tlist->semCount, 0, 1);	  		// Initialize it for Ready


    pthread_attr_init(&tlist->attr);
    pthread_attr_setdetachstate(&tlist->attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&tlist->thread, &tlist->attr, timerProc, (void *) tlist);

    return (RADLIST_ID) tlist;
}

void	timerDeleteList (RADLIST_ID list)
{
	struct timerlist * l = (struct timerlist *) list;
	Timer_id *t;
	for (t = (Timer_id*)radListRemoveLast(l->list);
			t != NULL;
			t=(Timer_id*)radListRemoveLast(l->list))
	{
		timerDelete(t);
	}


	pthread_cancel(l->thread);
	pthread_join(l->thread,NULL);
	pthread_attr_destroy(&l->attr);
	pthread_exit(NULL);
	sem_destroy(&l->semCount);

	free (l);
}

/*
 *  individual Timers
 */
Timer_id *   timerCreate(RADLIST_ID list, void (*callback)(void *work), void * work, int mSeconds)
{
	struct timerlist        *l = (struct timerlist *) list;
	Timer_id                * t;

	t = (Timer_id *) malloc(sizeof(Timer_id));
	if (t != NULL)
	{
		t->callback = callback;
		t->procwork = work;
		t->msecs 	= mSeconds;
		t->stopped  = 1;
		t->exit 	= 0; 			// Delete if exit != 0
		radListAddToEnd((RADLIST_ID)l->list, (NODE_PTR)t);
	}
	return t;
}

/*
 *
 */
void	timerDelete(Timer_id *t)
{
	// find t in list
	// stop t, remove t, free memory
	//
		t->stopped = 1; // stopp timer
		t->exit = 1;   // mark for deletion

}


/*
 *  timerStart(mylist, t, NULL) restart the timer with t->msecs
 *  timerStart(mylist, t, 200) restart timer using 200ms
 *  timerStop(t) stop the timer.
 *  return 0 on success -1 on failure
 */

int timerStart(RADLIST_ID list, Timer_id *t, int mSeconds)
{
	struct timerlist 	*tl = (struct timerlist *) list;
	int val=-1;
	printf ("timerStart-1, t=%x, tl=%x\n");
	if (t)
	{
		if (tl)
		{
			t->stopped = 0; // stopped = False ...run
			printf ("timerStart-2, t=%x, tl=%x\n");
			sem_wait(&tl->semCount);
				if (mSeconds>0)
					t->countdown = mSeconds;
				else
					t->countdown= t->msecs;
			sem_post(&tl->semCount);
		}
		val=0;
	}
    return (val);
}

int timerStop(Timer_id *t)
{
	int val= -1;

	if (t)
	{
		t->stopped = 1; // Stop = True
		val=0;
	}
	return val;
}





