	/* ***************************************************************************
	//  Copyright (c) 2011 Luminator Holding, LP
	//  All rights reserved.
	//  Any use without the prior written consent of Luminator Holding, LP
	//  is strictly prohibited.
	// ***************************************************************************

	  Filename:   buttonmon.c

	  Description:

	  Revision History:
	  Date             Name              Ver    Remarks
	  Apr 21, 2011     wwright           0
	  Feb 13, 2013     mlane             1      Adding support for button beeping


	  ToDo:
		  Re-write Serial interface, requires re-wire of products button.
		  serial is polling by writing tx, reading rx every uSleep(100000)
		  convert to interrupt based.
		  -- This was done for the DEMO, and not redone for product
		  --   because we went from Demo to Product with out any time -



	  Notes:       ...
			read config

			thread busApiRx()
			.. listen Messages from busAPI
				 read udp()
				 update arrivals
			  update alerts

		  thread playAudio()
		  ..waits for audio to be ready
			 while(1)
				 wait(sem)
				 if Alert
					 play alert
				 else if Arrival
					 play arrival


		  ButtonReleased()
			  buttonCnt = (buttonCnt+1)%3
			 state = idle
				  if buttonCnt==1
						 start timer(t1)
						 next state = Arrivals
					elseif buttonCnt == 2
						start timer(t1)
						next state = Alerts

					elseif buttonCnt == 3

			 state = Arrivals
			 state = Alerts


			thread buttonListener()
			..register button press, releases.
				while(1)
					 usleep(n)		//POLL
					 txSerial(fd)
					 read(fd)
					 if (string READ)
						 if (buttonUP)
							button pressed
					 else if (Read Error || not equal string)
						 if ( was buttonDown)
							 okay button released

			thread keyboard listener()
			..simulate a button and also process commands from key board, when in foreground
				 debugs
				 help
				 play audio
				 quit


		  Init work ()
			Init serial ()
				 open(serialFD)

		  main()
			  init
			  start threads
			  close down threads
			  exit.

	*  *****************************************************************************
	*/


	#include <sys/types.h>
	#include <sys/stat.h>
	#include <sys/mman.h>
	#include <sys/ioctl.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>

	#include <netdb.h>
	#include <pthread.h>
	#include <sched.h>
	#include <semaphore.h>
	#include <fcntl.h>
	#include <termios.h>
	#include <limits.h>

	#include <stdio.h>
	#include <stdlib.h>
	#include <stdarg.h>
	#include <strings.h>
	#include <string.h>
	#include <getopt.h>

	#include <time.h>
	#include <assert.h>
	#include <signal.h>

	#include <syslog.h>

	#include <tts.h>


	#define __USE_XOPEN_EXTENDED 1
	#include <unistd.h>
	#include <errno.h>

	#include <ctssTimer.h>
	#include <radlist.h>


	int buttonState = 0;
	#define die(msg) { perror(msg); go=0; exit(EXIT_FAILURE); }

	#define MAX_RX_BUF		4098
	#define AUDIO_BUF		8192
	#define ALERT_BUF		8192
	unsigned char         	recv_data[MAX_RX_BUF+1];
	unsigned char 			audiobuf[AUDIO_BUF];
	unsigned char 			alertbuf[ALERT_BUF];
	unsigned char 			ttsbuf[AUDIO_BUF];

	static char tstStrg[] = "999999999999999" ;

	#define VOICE_NAME	50
	#define TRACKER_CONF "../cfg/tracker.conf"

	#include <../common/trackerconf.h>

	#include "sys/syslog.h"

	/*
	  pthread_t ctlRxThread;
	  pthread_t ctlKeysThread;
	  pthread_t ctlTxThread;
	  */
	pthread_attr_t      attr;
	sem_t               arrival_mutex;
	sem_t               alert_mutex;
	sem_t               keyboardAudio_mutex;

	int         go = 1;

	//struct signData     sdata;

	typedef enum {BSM_idle, BSM_arrivals, BSM_alerts }ButtonStateMachine;
	typedef enum  {ButtonUp,ButtonDown,ButtonUnknown}ButtonState;
	#define BUTTON_MIN_POLL 	50000		// .05sec  9600b/s = 960B/s = 1 char/usec 13char = .013 Sec min
	#define BUTTON_MAX_POLL 	1000000 	// 1Sec  10^-6 x 10^6  - will be very sluggish
	#define BUTTON_DEFAULT_POLL 100000		// .1Sec

	typedef enum
	{
		 BUS_API_RX_THREAD_e  = 0,	// Messages from API, arrival, alerts.
		 TX_THREAD_e,
		 KEY_THREAD_e,				// Keyboard Monitor
		 AUDIO_KEY_THREAD_e,         // Wait to play audio after Keyboard 'a'
		 BUTTON_MONITOR_THREAD_e,    // monitor Serial line tied to a Button press
		 AUDIO_THREAD_e,    			// Play Audio

		 CTL_MAXTHRDS_e
	}CTL_THREAD_t;


	/*
	 *  Global Work AREA
	 *  extern struct TrackerData *gwork
	 *
	 */
	struct TrackerData
	{
		 int       		i;
		 int       		txStop;
		 int       		rxStop;

		 pthread_t 		thread[CTL_MAXTHRDS_e];                     // array of threads
		 void *    		(*proc[CTL_MAXTHRDS_e])(void *);            // array of func pointers.
		 int       		debug;      // 0,1 OFF,ON

		 char      		voice[VOICE_NAME];

		 Timer_id		*t1;
		 int				fd;
		 int       		*map;

		 int       		buttonFp;
		 struct termios  portConfig;

		 char      		trackerConf[512];
		 RADLIST_ID		tlist;
		ButtonStateMachine state;
		ButtonStateMachine nextState;
		sem_t			semPlay;
		int				buttonTaps;

		int				buttonTimeout;


	};
	struct TrackerData *gwork;

	/* Local Function(s) */
	void setButtonVolume(char volume) ;

	/* - - - - - - - - - - - - -
	 *   debug()
	 *     print statement if debug true.
	 */
	int debug (int level, char *fmt, ...)
	{
	  va_list   list;
	  int       r;
	  int priority = LOG_CONS | LOG_PID;

	  if (gwork == NULL) return 0;

	  if (gwork->debug < level)
	  {
		 va_start(list,fmt);
		 r = vprintf(fmt, list);
		 vsyslog (priority, fmt, list);
		 va_end(list);
	  }
	}

	/* - - - - - - - - - - - - -
	 * int readConfig()
	 *  - read tracker.conf  properties file. (key,value)
	 *
	 */
	int readConfig()
	{
		 /*read configfile
		 * get interface connected to the Sign eth0,eth1,eth2,eth3
		 */

		 FILE    *fp;
		 char    key[255], value[255], line[255];
		 int     rval, linlen,keylen,vallen;
		 char    *xstr;

		 debug (2,"readConfig()\n");

		 fp = fopen(gwork->trackerConf,"r");
		 if (fp != NULL)
		 {
			  xstr = fgets(line, 255, fp);
			  while ( xstr != NULL )
			  {
					if ((line[0]  != '#' && line[0] != '\n' ))
					{
						 rval = sscanf(line,"%s ", key);
						 keylen = strlen( (const char*)key );
						 linlen =  strlen( (const char*)line);

						 if (keylen+1 == linlen)
							  strcpy (value,"");
						 else
						 {
							  strncpy(value, (const char*) &line[keylen+1], linlen-(keylen)+1);
							  vallen = strlen(value);
							  //printf("vallen=%d value[vallen-1]  = 0x%02x \n",vallen-1,value[vallen-1]);
							  if (value[vallen-1] == '\n')
									value[vallen-1] = '\0';
						 }
						 if (putconfig(key,value))
							  debug (4,"Error adding key,value to config %s,%s\n",key,value);

						 //printf ("key=%s, value=%s\n",key,value);
					}
					xstr = fgets(line, 255, fp);
			  }

			  fclose (fp);
		 }
	}


	/* - - - - - - - - - - - - -
	 *      void * ctlTx(void * work)
	 *      - send sign data.
	 */
	void * ctlTx(void * work)
	{
	}


	/* - - - - - - - - - - - - -
	 *      void updateArrivalTTS()
	 *      - update the Audio TTS buffer.
	 */
	void updateArrivalTTS( int len, unsigned char * msg)
	{
		 char arrival_txt[512];
		 char line[255];
		 FILE *fp;

		 // Get path/filename
		 strncpy(arrival_txt, getconfig("audio_file"), sizeof(arrival_txt));

		 debug(1,"Arrival: updateArrivalTTS(%d,%x,%c)",len,msg[0],msg[1]);

    sem_wait(&arrival_mutex);
		memset(audiobuf,0,sizeof(audiobuf));
		strcat ( audiobuf,"'");
		memcpy ( &audiobuf[1], &msg[1], len);  		// skip the message type=Arrival
		strcat ( audiobuf,"'");
    sem_post(&arrival_mutex);

    // Save data to a file.
    fp = fopen (arrival_txt, "wb");
    if (fp == NULL)
    {
    	debug(1,"failed open %s \n",arrival_txt);
        perror(arrival_txt);
        return;
    }
    fwrite (&msg[1], sizeof(unsigned char), len, fp);
    fclose (fp);
}

/* - - - - - - - - - - - - -
 *      void updateAlertsTTS()
 *      - update the Audio TTS buffer.
 */
void updateAlertsTTS( int len, unsigned char * msg)
{
    char alert_txt[512];
    char line[255];
    FILE *fp;

    // Get path/filename
    strncpy(alert_txt, getconfig("audio_alert_file"), sizeof(alert_txt));

    if (len > 4)
    	debug(1,"Alert: updateAlertsTTS(%d,%x,%x,%x,%c)",len,msg[0],msg[1],msg[2],msg[3]);
    else
    	debug(1,"Alert: updateAlertsTTS(%d,%x,%x,%x)",len,msg[0],msg[1],msg[2]);

    sem_wait(&alert_mutex);
		memset(alertbuf,0,sizeof(alertbuf));
		strcat ( alertbuf,"'");
		memcpy ( &alertbuf[1], &msg[AUDIO_DATA_START], len);  		// skip the message type=Arrival
		alertbuf[len]='\0';
		strcat ( alertbuf,"'");
    sem_post(&alert_mutex);

    // Save data to a file.
    fp = fopen (alert_txt, "wb");
    if (fp == NULL)
    {
    	debug(1,"failed open %s \n",alert_txt);
        perror(alert_txt);
        return;
    }
    fwrite (&msg[AUDIO_DATA_START], sizeof(unsigned char), len, fp);
    fclose (fp);
}



/* - - - - - - - - - - - - -
 *  void processMsg( int len, unsigned char * msg)
 */
void processMsg( int len, unsigned char * msg)
{
    debug(1,"recieve len=%d\n",len);
    switch (msg[0])
    {
    case BusArrival:
    	updateArrivalTTS(len, msg);
    	break;
    case BusAlert:
    	updateAlertsTTS(len, msg);
    	break;
    case BusError:
    	break;
    case BusConfig:
    	break;
    }
}

/* - - - - - - - - - - - - -
 *      void *busApiRx(void * work)
 *      - Listen for signal from busapi.py
 *      - udp message pend (port) 'update' info
 */

void *busApiRx(void * mywork)
{
    /* Listen for messages
    *
    */

    int                   sock;
    struct hostent        *host;
    struct sockaddr_in    bindaddr , rxaddr;
    int                   addr_len, bytes_read;
    char                  addrtext[32];
    socklen_t             socklen;
    struct sockaddr_in    addr;

    struct TrackerData *work = (struct TrackerData *) mywork;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        debug(1,"Socket error\n");
        perror("Socket");
        exit(1);
    }

    // Get configfile port number to recieve on.
    int port = atoi(getconfig("tracker_audio_port"));

    // Range check port from config file.
    if (port<1000 || port >= USHRT_MAX)
        port = LISTEN_PORT+1;   // out of range set to default audio port is +1 from display port.

    debug(1,"port=%d, maxshort=%u\n ", port, USHRT_MAX);

    bindaddr.sin_family = AF_INET;
    bindaddr.sin_port = htons(port);
    bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&(bindaddr.sin_zero),8);

    if (bind(sock,(struct sockaddr *)&bindaddr,sizeof(struct sockaddr)) < 0 )
    {
        perror("Bind");
        exit(1);
    }

    addr_len = sizeof(struct sockaddr);

    //  pthread_join(sbTxThread,    NULL);
    work->txStop = 1;

    debug (2,"busApiRx() wait on %d\n", port);

    while (go)
    {
        //Pend wait for Signal from python.
        bytes_read = recvfrom(sock,recv_data, MAX_RX_BUF-1, 0,
                    (struct sockaddr *)&rxaddr, &addr_len);
        if (bytes_read < 0)
        {
            debug(1,"Read message Len <0, exit\n");
            perror("Read message Len < 0, exit");
            break;
            /*
             * TODO: how to handle ERROR here. Try()Catch()
             * - re-try the socket. or use process Monitor to restart
             */
        }
        recv_data[bytes_read] = '\0';
        processMsg(bytes_read, recv_data);

        fflush(stdout);
        if (work->rxStop == 0)
        {
            debug (2,"RX STOP \n");
            break;
        }
    }
    // Tell TX to stop
    work->txStop = 0;
    if (work->thread[TX_THREAD_e])
    {
        pthread_join(work->thread[TX_THREAD_e], NULL);
    	pthread_exit(NULL);
    }

    printf ("RX thread exit \n");

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
   "\n 1,a,A    = Audio TTS ",
   "\n 3,d,D    = toggle debugs",
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

    struct TrackerData *work = (struct TrackerData *) workstruct;



    while(go)
    {
      val = scanf("%10s",cmd);
      key = (int) cmd[0];
      switch (key)
      {

        case 0x48:      //h,H,?
        case 0x68:
        case 0x03f :
              // print Help
              debug (1,"command = 0x3f\n");
              for (i=0; (int)*helpcmds[i] != (int)NULL; i++)
                  printf ("%s", helpcmds[i]);
              break;
        case 0x51:
        case 0x71: //'q' 'Q' :
            // Exiting Request Quit
            //  exit thread
            go = 0;
            printf (" Quit \n");
            break;

        case 0x31: // '1' 'a' 'A':
        case 0x41: // '1' 'a' 'A':
        case 0x61: // '1' 'a' 'A':
            // Play Audio
        	sem_post(&keyboardAudio_mutex);
            break;

        case 0x32: //'2':
            break;

        case 0x44:  // d, D, 3
        case 0x64:
        case 0x33: //'3':
            //    printAsciiSign();
            // debug Levels. 0,1,2,3,4   (debug < level)  level=1,2,3,4,5 - 5 always gets printed.
            printf ("Increase/toggle Debug++ \n");
            work->debug = ((work->debug + 1) % 5) ;
            printf ("debug=%d\n",work->debug);
            break;

        default:

          break;

      }

    }
  printf ("exit Keyproc()\n");
  pthread_exit(NULL);

}


/* - - - - - - - - - - - - -
 *      void * audioTts(void * work)
 *       - if keyboard pressed play audio text
 *       - Debugging if we do not have a "button" connected to Serial.
 */
#define VOICE_COUNT     2
static char * tts_voices [] = {"Allison", "David"};
static char player[] = "/usr/local/bin/swift";
char file[] = "./arrival.txt";  //default.
char tempfile[] = "/tmp/tempF.txt";

void * audioKeyTts(void * mywork)
{
    int         code;
    int         am_val;
    char        arrival_txt[255];
    char        command[1024];

    struct TrackerData *work = (struct TrackerData *) mywork;

    strncpy(audiobuf,"Waiting for Update",sizeof(audiobuf));

    debug (1,"audio_file=%s\n",getconfig("audio_file"));
    strncpy(arrival_txt, getconfig("audio_file"), sizeof(arrival_txt));
    snprintf( command, sizeof(command), "%s -n %s -f %s ", player, (char *) work->voice, arrival_txt);

    debug (1,"audio_txt=%s, strlen(arrival_txt)=%d\n",arrival_txt,strlen(arrival_txt));
    if (strlen(arrival_txt) == 0 )
    {
        debug(4,"In (tracker.conf) config file audio_file is NULL, Edit and set to a valid FileName\n");
        printf ("exit audioTts()\n");
        pthread_exit(NULL);
    }

    while(go)
    {
    	// wait for keyboard '1' 'a' or 'A'
    	sem_wait(&keyboardAudio_mutex);

    	// Copy Arrival message
//        sem_wait (&arrival_mutex);
    		memcpy ( ttsbuf, audiobuf, sizeof(ttsbuf));
//        sem_post (&arrival_mutex);

        // read Message
        debug(1,"Play thou audio... then wait...cnt=%d\n", am_val);
        snprintf( command, sizeof(command), "%s -n %s %s ", player, (char *) work->voice, ttsbuf);
        code = system (command);
    	sleep(3);
    }

    printf ("exit audioTts()\n");
    pthread_exit(NULL);
}



/* - - - - - - - - - - - - -
 *   void * buttonTimeout(int sig)
 *     - if Serial Button NOT pressed in time
 *     - clear button State
 */
static void buttonTimeout(int sig)
{
	buttonState = 0;
} /* buttonTimeout */


/* - - - - - - - - - - - - -
 *   void * playAudio(void * work)
 *     - call "swift" to play audio.
 */
void * playAudio(void *mywork)
{
    struct TrackerData *work = (struct TrackerData *) mywork;

    debug (1,"Enter playAudio()\n");

    while(1)
    {
    	sem_wait(&work->semPlay);

		switch (work->state)
		{
			case BSM_arrivals:
				debug(1,"Play Arrivals \n");
				// Get current data
				sem_wait (&arrival_mutex);
					memcpy ( ttsbuf, audiobuf, sizeof(ttsbuf));
				sem_post (&arrival_mutex);
				break;
			case BSM_alerts:
				debug(1,"Play Alerts \n");
				// Get current data
				sem_wait(&alert_mutex);
					memcpy ( ttsbuf, alertbuf, sizeof(ttsbuf));
				sem_post(&alert_mutex);
				break;
		}

		switch (work->state)
		{
			case BSM_arrivals:
			case BSM_alerts:
				ttsPlay (ttsbuf);
				work->state = work->nextState;
				break;
		}
    }

    debug(1,"Finish Playing.");
    pthread_exit(NULL);
}



void startPlayer(struct TrackerData * work)
{
	printf("start Player process\n");
	sem_post(&work->semPlay);
}

void stopPlayer(struct TrackerData * work)
{
	printf("Stop Player process\n");
	ttsStop();
}

void timer_handler( void *mywork )
{
	struct TrackerData *work = (struct TrackerData *) mywork;

	debug(1,"Enter. timer_handler() nextstate=%d\n", work->nextState);
	work->buttonTaps = 0;

	// TIMER went off
	switch (work->nextState)
	{
		case BSM_idle:
			//
			work->state = BSM_idle;
			break;

		case BSM_arrivals:
			// timer expired waiting for 2Presses.
			// play Arrivals
			printf("startPlayer()");
			work->state = BSM_arrivals;
			startPlayer (work);
			work->nextState=BSM_idle;
			break;

		case BSM_alerts:
			// monitor 3Press
			work->state = BSM_alerts;
			startPlayer(work);
			work->nextState=BSM_idle;
			break;
	}
	work->buttonTaps = 0;
}

/*
 * Transition from UP to Down
 */
void buttonPressed( struct TrackerData *work)
{
	// WE DONT CARE when button is down only rising edge.
	debug(1,"Button Pressed\n");
}

/*
 * Transition from Down to UP
 */
void buttonReleased( struct TrackerData *work)
{

	// 0,1,2,3
	work->buttonTaps = ( (work->buttonTaps+1) % 4);
	timerStop(work->t1);

	debug(1,"Entry Button Released state=%d, taps=%d\n",work->state, work->buttonTaps);

	// State machine.
	switch (work->state)
	{
		case BSM_idle:
			// monitor 1Press, 2Press
			if (work->buttonTaps == 1)
			{
				debug(1,"1\n");
				work->nextState = BSM_arrivals;
				debug(1,"2\n");
				timerStart(work->tlist, work->t1, 0);
			}
			else if (work->buttonTaps == 2)
			{
				debug(1,"3\n");
				work->nextState = BSM_alerts;
				debug(1,"4\n");
				timerStart(work->tlist, work->t1, 0);
			}
			else if (work->buttonTaps == 3)
			{
				debug(1,"5x\n");
				timerStop(work->t1);
				debug(1,"6x\n");
				stopPlayer(work);
				debug(1,"7x\n");
				work->nextState = BSM_idle;
				work->state =  BSM_idle;
				work->buttonTaps = 0;
			}
			break;

		case BSM_arrivals:
			// monitor 3Press
			if (work->buttonTaps == 2)
			{
				timerStop(work->t1);
				stopPlayer(work);
				debug(1,"8\n");
				work->nextState = BSM_alerts;
				debug(1,"9\n");
				timerStart(work->tlist, work->t1, 0);
			}
			else if (work->buttonTaps == 3)
			{
				//Stop play of Arrivals
				timerStop(work->t1);
				stopPlayer(work);
				work->state = BSM_idle;
				work->buttonTaps = 0;
			}

			break;

		case BSM_alerts:
			// monitor 3Press
			if (work->buttonTaps == 3)
			{
				//Stop play of Arrivals
				timerStop(work->t1);
				stopPlayer(work);
				work->state = BSM_idle;
				work->buttonTaps = 0;
			}
			else
			{
				work->state = BSM_idle;
				work->buttonTaps = 0;
			}
			break;
		default:
			debug(1,"Breleased: State=%d, nextstate=%d \n",work->state, work->nextState);
	}
	debug(1,"Exit Button Released\n");
}



/* - - - - - - - - - - - - -
 *   void * buttonMonitor(void * work)
 *     - Simply monitor Button presses.
 */

void * buttonMonitor(void * mywork)
{
    char        	command[1400];
    int         	code;
    int     	    am_val;
    char   	    	arrival_txt[255];
    char            rcvStrg[32];
	int				s;
	ButtonState		pstate, cstate;

	pstate = cstate = ButtonUnknown;
	ttsInit();

    struct TrackerData *work = (struct TrackerData *) mywork;

    debug (1,"Button: audio_file=%s\n",getconfig("audio_file"));
    debug (1,"Button: Start thread  \n");

    // getconfig("tracker_button_readdelay");
    // getconfig("tracker_button_timeout");
    int buttonReadDelay = atoi(getconfig("tracker_button_readdelay"));

    if (buttonReadDelay < BUTTON_MIN_POLL || buttonReadDelay > BUTTON_MAX_POLL)
    	buttonReadDelay = BUTTON_DEFAULT_POLL;

    // SET STATE = 0   Initial button press.
    buttonState = 0;

	 memset( tstStrg, '\0', sizeof(tstStrg) ) ;
	 memset( tstStrg, '9', sizeof(tstStrg) - 1 ) ;

    // loop forever - later add condition test to exit
    debug (1, "go=%d\n",go);
    while (go)
    {
        // delay 1/100 second between polls .000 000 1
        usleep(buttonReadDelay);

        // poll the switch
        write(gwork->buttonFp, tstStrg, strlen(tstStrg));
        // clear buffer
        memset(rcvStrg, 0, 32);
        // if we don't get a full buffer
        if (read(gwork->buttonFp, rcvStrg, strlen(tstStrg)) <= 0)
        {
    		rcvStrg[31]=0;
        	if ( strlen(rcvStrg)>0)
        	{
            	debug(1,"button loop, %s",rcvStrg);
        	}
            // flush the buffer
            while(read(gwork->buttonFp, rcvStrg, strlen(tstStrg)) > 0);

			cstate = ButtonUp;
        	switch (pstate){
				case ButtonUp:
					break;
				case ButtonDown:
					buttonReleased(work);
					break;
				case ButtonUnknown:
					break;
        	}
        	pstate=cstate;

        	// go poll again
            continue;
        }

        //debug(1,"\n...rcv=%s, tstStrg=%s\n",rcvStrg,tstStrg);

        // if we get the right string in
        if (strncmp(rcvStrg, tstStrg, sizeof(tstStrg)) == 0)
        {

			cstate = ButtonDown;
        	switch (pstate){
				case ButtonUp:
					buttonPressed(work);
					break;
				case ButtonDown:
					break;
				case ButtonUnknown:
					break;
        	}
        	pstate=cstate;

            // flush the buffer
            while(read(gwork->buttonFp, rcvStrg, strlen(tstStrg)) > 0);

        }
        // debug(1,"flush2\n");
        // flush the buffer
        while(read(gwork->buttonFp, rcvStrg, strlen(tstStrg)) > 0);
    }

    // close the port
    close(gwork->buttonFp);
    ttsExit();
}

/* - - - - - - - - - - - - -
 *  int serialInit()
 *  - initialize the serial port using configdata from tracker.conf
 *
 */
int serialInit()
{
    // generate ttyS port string
    //sprintf(port, "/dev/ttyS%d", portI);
    char            *port;


    port = getconfig("tracker_button");
	debug (1, "Serial Initialization");

    //strncpy (port, getconfig("tracker_button"),sizeof(port));

    if (port != NULL)
    {
		debug (1, "port=%s\n",port);
		// open ttyS port
		gwork->buttonFp = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
		// port opened fine
		if (gwork->buttonFp <= 0)
		{
			debug(1,"Exit button error\n");
			fprintf(stderr,"Could not open port -%s-\n", port);
			exit(1);
		}

		tcgetattr (gwork->buttonFp, &gwork->portConfig);
		tcflush (gwork->buttonFp, TCIFLUSH);
		gwork->portConfig.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
		gwork->portConfig.c_oflag &= ~OPOST;
		gwork->portConfig.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
		gwork->portConfig.c_cflag &= ~(CSIZE | PARENB);
		gwork->portConfig.c_cflag |= CS8;


		// Set the input and output baud rates to 460,800 bits/sec.
		// We ignore what's in config data and force it manually.
		cfsetispeed (&gwork->portConfig, B9600);
		cfsetospeed (&gwork->portConfig, B9600);
		tcsetattr (gwork->buttonFp, TCSANOW, &gwork->portConfig);
		debug (1, "Normal exit serial init. buttonFp=%d\n",gwork->buttonFp);
		//fprintf(stderr, "        Type Ctrl-C to exit.\n");
		return 0;
    }
    return 1;
}



/* - - - - - - - - - - - - -
 *      void workinit(struct TrackerData * work)
 *      - Initialize work area
 *      - read config
 *      - serial init
 *      - get Voice
 *      - set thread processes, attr, work
 */
void workinit(struct TrackerData * work)
{
    char voice[VOICE_NAME];
    int i;

    configinit();
    readConfig();
    dumpconfig();

    work->buttonFp = -1;
    work->fd = -1;
	work->state = BSM_idle;

    if (serialInit())
    {
    	debug(4,"Error Initializing SERIAL port=%s , Check config file for entry 'tracker_button' \n",getconfig("tracker_button"));
    	exit(1);
	}

    work->tlist = timerCreateList(1000);	// 1 mSec List.
    if (work->tlist == NULL)
    {
    	debug(1,"Failure to create timerList.");
    	exit(1);
    }

    int bto = atoi(getconfig("tracker_button_timeout"));
    work->buttonTimeout = (bto<500 || bto > 5000)?1000:bto;
    if (bto<500 || bto > 5000)
    {
    	debug(1,"Error Sanity check failed on buttonTimeout defaulted to 1000ms\n");
    }

    work->t1 = timerCreate (work->tlist, timer_handler, (void *)work, work->buttonTimeout); // 1000 mSec Timer
	if (work->t1 == NULL)
	{
		printf ("timerCreate return NULL\n");
		radListDelete(work->tlist);
	    exit (1);
	}


    //Initialize the semaphore in the main function:
    debug(1,"sem_init\n");
    sem_init(&arrival_mutex, 0, 1);	  		// Initialize it for Ready
    sem_init(&alert_mutex, 0, 1);
    sem_init(&keyboardAudio_mutex, 0, 0);	// keyboard input to test/instead of serial port
	sem_init(&work->semPlay, 0, 0);			// Not ready, wait for button press.

    strncpy(audiobuf,"Waiting for Update",sizeof(audiobuf));
    strncpy(alertbuf,"There are No, Alerts", sizeof(alertbuf));


    // Config Audio TTS
    debug(1,"voice init get %s\n",getconfig("voice"));
    strncpy(voice,getconfig("voice"), sizeof(voice));
    strncpy(work->voice, tts_voices[1], sizeof(voice)); //default to David
    for (i=0; i<2; i++)
        debug(1,"voice[%d]=%s\n",i,tts_voices[i]);
    debug(1,"voice init buf %s\n",work->voice);

    if (strlen (voice) != 0)
    {
        debug(1,"voice1, %s\n",work->voice);
        for ( i=0; i< VOICE_COUNT; i++)
        {
            debug(1,"voice2, %s\n",work->voice);
            if (strncasecmp(tts_voices[i], voice, strlen(tts_voices[i])) == 0 )
                strncpy(work->voice,tts_voices[i],sizeof(work->voice));
        }
        debug(1,"voice3, %s\n",work->voice);
    }

    debug(1,"init Process ptr\n");
    // Init Thread Procedures
    for ( i=0 ; i<CTL_MAXTHRDS_e; i++)
    {
        switch (i)
        {
        case BUTTON_MONITOR_THREAD_e:
            work->proc[BUTTON_MONITOR_THREAD_e] = buttonMonitor;
            break;
        case BUS_API_RX_THREAD_e:
            work->proc[BUS_API_RX_THREAD_e] = busApiRx;
            break;
        case TX_THREAD_e:
            work->proc[TX_THREAD_e] = ctlTx;
            break;
        case KEY_THREAD_e:
            work->proc[KEY_THREAD_e] = keyproc;
            break;
        case AUDIO_KEY_THREAD_e:
            work->proc[AUDIO_KEY_THREAD_e] = audioKeyTts;
            break;
        case AUDIO_THREAD_e:
        	work->proc[AUDIO_THREAD_e] = playAudio;
        	break;
        }
    }
}

void readCommandLine (int argc, char *argv[])
{
    int opt;
    int i;
    extern char *optarg;

    while ((opt = getopt(argc, argv, "d:c:")) != -1)
    {
        switch (opt)
        {
        case 'c':
            snprintf(gwork->trackerConf,sizeof(gwork->trackerConf),"%s",optarg);
            break;

        case 'd':
            i = atoi(optarg);
            if (i >= 0)
                gwork->debug = i;
            break;

        default: /* '?' */
            fprintf(stderr, "Usage: %s [-c configFile] [-d debugLevel]\n",
                argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}

void buttonShutdown(struct TrackerData  *work)
{
	int i;
	printf ("Wait for KEY 'q'\n");
	pthread_join(work->thread[KEY_THREAD_e], NULL);

    for ( i=0 ; i<CTL_MAXTHRDS_e; i++)
    {
    	if (work->thread[i])
    		pthread_cancel(work->thread[i]);

    	if ( i != KEY_THREAD_e)
    		pthread_join(work->thread[i], NULL);

	}

	sem_destroy(&work->semPlay);

	free (work);
	sem_destroy(&arrival_mutex);
	sem_destroy(&alert_mutex);
	sem_destroy(&keyboardAudio_mutex);
}

/* - - - - - - - - - - - - -
 *      int main(argc,argv)
 *      - MAIN entry point.
 *      - process argv cmd line
 *      - Initialize
 *      - start threads, keys, audioKeyTTS, audioButton, UDP rx from busapi.py,
 */
int main (int argc, char **argv)
{

    int                 rc=0;
    struct TrackerData  *work;
    int                 i;

    work = (struct TrackerData *) malloc (sizeof (struct TrackerData));
    gwork = work;

    work->rxStop = 1;  //Go
    work->debug = 0;   // DEFAULT print everything.

    snprintf(work->trackerConf, sizeof(work->trackerConf),"%s", TRACKER_CONF);
    readCommandLine (argc, argv);

    /* Initialize and set thread detached attribute */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    printf ("debug=%d \n", gwork->debug);
    workinit(work);

    for (i=0; i<CTL_MAXTHRDS_e; i++)
    {
    	//Start your engines..
        debug(1,"start process %i",i);
        pthread_create(&work->thread[i], &attr, work->proc[i], (void *) work);
    }

    pthread_join(work->thread[BUTTON_MONITOR_THREAD_e], NULL);

    printf ("\nnormal termination.\n");
    buttonShutdown(work);
    pthread_attr_destroy(&attr);
    pthread_exit(NULL);
}


/* 
 * -----------------------------------------------------------------------------
 *  void setButtonVolume(char volume)
 *     This routine will set the button beep volume. The volume range is 
 *     '0' - '9'. A volume level of '0' will mute the one-second beep.
 * ----------------------------------------------------------------------------- 
 */
void setButtonVolume(char volume)
{
	if ( volume >= '0' && volume <= '9' )
	{
		memset( tstStrg, '\0', sizeof(tstStrg) ) ;
		memset( tstStrg, volume, sizeof(tstStrg) - 1 ) ;

		write(gwork->buttonFp, tstStrg, strlen(tstStrg));
	}
}
