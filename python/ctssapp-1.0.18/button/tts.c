/* tts_example.c: Simple Text to Speech using Swift.
 *
 * Copyright (c) 2004-2006 Cepstral, LLC.  All rights reserved.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS".  CEPSTRAL, LLC DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * CEPSTRAL, LLC BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* This example program provides the simplest possible example of
 * using the Swift API to perform text-to-speech.  It opens an 
 * engine, opens a port, and uses the default voice to
 * synthesize each command line argument, playing to the default
 * audio device.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <strings.h>
#include <string.h>
#include <semaphore.h>

#include <swift.h>

const char usage[] = "Usage: tts_example STRING [STRING...]\n\n";

swift_engine 		*engine;
swift_port 			*port = NULL;
swift_background_t 	tts_stream;
int stopflag = 0;

sem_t               tts_mutex;

extern void setButtonVolume(char volume) ;

int ttsInit()
{
    /* Open the Swift TTS Engine */
    if ( (engine = swift_engine_open(NULL)) == NULL)
    {
        fprintf(stderr, "Failed to open Swift Engine.");
        return 1;
    }
    sem_init(&tts_mutex, 0, 1);	  		// Initialize it for Ready
    return 0;
}

void ttsExit()
{
	if (engine != NULL)
		swift_engine_close(engine);
	engine = NULL;
	sem_destroy(&tts_mutex);
}

int ttsStop()
{
	printf ("ttsStop() port=0x%x,engine=0x%x\n",port, engine);

    sem_wait(&tts_mutex);

		if (port != NULL && SWIFT_STATUS_RUNNING == swift_port_status(port, tts_stream))
		{
			printf ("ttsStop(), calling port stop, port_wait\n");
			swift_port_stop(port, tts_stream, SWIFT_EVENT_NOW);
			swift_port_wait(port, tts_stream);
		}

		if ( port  != NULL)
		{
			printf ("ttsStop(), 2 calling port CLOSE\n");
			swift_port_close(port);
		}
		port = NULL;
    sem_post(&tts_mutex);

    /* unmute button beep */
	 setButtonVolume('9') ;
}

int ttsPlay(char *text)
{


    if (strlen(text)==0)
    {
        fprintf(stderr, usage);
        return 1;
    }

    /* Open a Swift Port through which to make TTS calls */
    if ( (port = swift_port_open(engine, NULL)) == NULL)
    {
        fprintf(stderr, "Failed to open Swift Port.");
        swift_engine_close(engine);
        return 1;
    }
	printf ("ttsPlay() port=0x%x,engine=0x%x\n", port, engine);

	 /* mute button beep */
	 setButtonVolume('0') ;

    /* Synthesize each argument as a text string */
    swift_port_speak_text(port, text, 0, NULL, &tts_stream, NULL);
    swift_port_wait(port,tts_stream);
    ttsStop();
    
    return 0;
}

