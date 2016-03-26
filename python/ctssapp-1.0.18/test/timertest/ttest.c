
//Are you expecting something like below

//#define _GNU_SOURCE 1

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <curses.h>

#include <ctssTimer.h>



typedef struct
{
	pthread_attr_t		attr;
	pthread_t 			thread;
	Timer_id			*t1;
	Timer_id			*t2;
	Timer_id			*t3;
	Timer_id			*t4;
}work;
RADLIST_ID	timerlist;


work mywork;

int GO = 5;
Timer_id 	*t;

void my_handler( void *pwork )
{
	work *mywork = (work *) pwork;

	printf("\nHello World, go=%d\n", GO);
	GO=GO-1;

	switch (GO)
	{
		case 4:
		  break;
		case 3:
		  timerStop(mywork->t1);
		  sleep (3);
		  timerStart(timerlist,mywork->t1, 0);
		  break;
		default:
			timerStart(timerlist,mywork->t1, 0);
			break;
	}

}
void my_handler2( void *pwork )
{
	work *mywork = (work *) pwork;
	printf ("timer2 handler \n");
	timerStart(timerlist, mywork->t2,0);
}
void my_handler3( void *pwork )
{
	work *mywork = (work *) pwork;
	printf ("timer3 handler \n");
	timerStart(timerlist, mywork->t3,0);
}
void my_handler4( void *pwork )
{
	work *mywork = (work *) pwork;
	printf ("timer4 handler \n");
	timerStart(timerlist, mywork->t4,0);
}
/* - - - - - - - - - - - - -
 *   void * keyproc(void *workstruct)
 *    - process keyboard commands from terminal
 *
 */
char *helpcmds[] =
  { "cmd = read CHAR from keyboard\n",
   "\n q,Q      = quit",
   "\n h,H,?    = help",
   "\n s,S,     = Stop",
   "\n g,G      = Go",
   " \n",
   ""
  };

void * keyproc(void *workstruct)
{
    char        cmd[10];
    int         i;
    int         key;
    int         val;
    int         am_val;

    work *mywork = (work *) workstruct;



    while(GO)
    {
		//val = scanf("%10s",cmd);
		key = (int) getchar();
		switch (key)
		{
			case 0x73: // 's' 'S'
			case 0x53:
				val = timerStop(mywork->t1);
				printf ("timerStop 1,2,3,4 \n");
				timerStop(mywork->t2);
				timerStop(mywork->t3);
				timerStop(mywork->t4);

				break;

			case 0x48:      //h,H,?
			case 0x68:
			case 0x03f :
				  // print Help
				  for (i=0; (int)*helpcmds[i] != (int)NULL; i++)
					  printf ("%s", helpcmds[i]);
				  break;
			case 0x51:
			case 0x71: //'q' 'Q' :
				// Exiting Request Quit
				//  exit thread
				GO = 0;
				printf (" Quit \n");
				break;


			case 0x47: // 'g' 'G'
			case 0x67:
				val = timerStart(timerlist, mywork->t1,0);
				timerStart(timerlist, mywork->t2,0);
				timerStart(timerlist, mywork->t3,0);
				timerStart(timerlist, mywork->t4,0);

				printf ("Start val=%d\n",val);
				break;

			default:
			  break;
		}
    }
  printf ("exit Keyproc()\n");
  pthread_exit(NULL);
}


int main (int argc, char **argv)
{

	timerlist = timerCreateList(1000); // One millisecond Timer list, List, wake up and decrement timers every ms.
	mywork.t1 = timerCreate (timerlist, my_handler, &mywork, 2000);	//Create Timer, 2sec timer.
	mywork.t2 = timerCreate (timerlist, my_handler2, &mywork, 4000);	//Create Timer, 2sec timer.
	mywork.t3 = timerCreate (timerlist, my_handler3, &mywork, 1000);	//Create Timer, 2sec timer.
	mywork.t4 = timerCreate (timerlist, my_handler4, &mywork, 8000);	//Create Timer, 2sec timer.

	if (mywork.t1 == NULL)
	{
		printf ("timerCreate return NULL\n");
	    exit (1);
	}

    if (system == NULL)
    {
    	// call timerDelete() for each timer put on the list.
    	timerDeleteList(timerlist);
    	free(mywork.t1);
    	free(mywork.t2);
    	free(mywork.t3);
    	free(mywork.t4);

    	exit(1);
    }

    pthread_attr_init(&mywork.attr);
    pthread_attr_setdetachstate(&mywork.attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&mywork.thread, &mywork.attr, keyproc, (void *) &mywork);

    printf ("start..\n");
	while (GO)
	{
//		for (int i=0; i<100000; i++){}
//		printf(".");
	}

    pthread_kill(mywork.thread,SIGKILL);

	printf ("Cancel work thread\n");
	pthread_cancel(mywork.thread);

	printf ("Join thread\n");
    pthread_join(mywork.thread, NULL);

	timerStop(mywork.t1);
	timerStop(mywork.t2);
	timerStop(mywork.t3);
	timerStop(mywork.t4);
	timerDeleteList(timerlist);

	printf ("\nExit \n");
	if (fork()) exit(0);

}
