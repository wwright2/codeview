//

#ifndef CTSS_TIMER
#define CTSS_TIMER 1

#include <radlist.h>

// HIDDEN
struct timer_id
{
	NODE				node;
	void 				(*callback)(void *);
	void				*procwork;
	int					msecs;
	int					countdown;
	int					stopped;
	int					exit;	// Delete Me if 1
};

// END HIDDEN

typedef struct timer_id  Timer_id;

/*
 *  List of all timers for individual timer lists.
 *   Creates a Thread to manage a timer list.
 */
extern RADLIST_ID	timerCreateList(int uSecondsTick);
extern void			timerDeleteList(RADLIST_ID list);

/*
 *  individual Timers
 */
extern Timer_id *   timerCreate(RADLIST_ID list, void (*callback)(void *work), void * work, int mSeconds);
extern void 		timerDelete(Timer_id *t);

/*
 *  timerStart( t, NULL) restart the timer with t->msecs
 *  timerStart( t, 200) restart timer using 200ms
 *  timerStop(t) stop the timer.
 *  return 0 on success -1 on failure
 */
extern int	 		timerStart(RADLIST_ID list, Timer_id *t, int mSeconds);
extern int 			timerStop(Timer_id *t);

#endif
