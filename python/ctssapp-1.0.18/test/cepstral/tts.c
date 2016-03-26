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
#include "swift.h"

const char usage[] = 
"Usage: tts_example STRING [STRING...]\n\n";

int main(int argc, char *argv[])
{
    swift_engine *engine;
    swift_port *port = NULL;
    int i;

    if (1 == argc) {
        fprintf(stderr, usage);
        exit(1);
    }

    /* Open the Swift TTS Engine */
    if ( (engine = swift_engine_open(NULL)) == NULL) {
        fprintf(stderr, "Failed to open Swift Engine.");
        goto all_done;
    }
    /* Open a Swift Port through which to make TTS calls */
    if ( (port = swift_port_open(engine, NULL)) == NULL) {
        fprintf(stderr, "Failed to open Swift Port.");
        goto all_done;
    }

    /* Synthesize each argument as a text string */
    for (i = 1; i < argc; ++i) {
        swift_port_speak_text(port, argv[i], 0, NULL, NULL, NULL); 
    }
    
all_done:
    /* Close the Swift Port and Engine */
    if (NULL != port) swift_port_close(port);
    if (NULL != engine) swift_engine_close(engine);
    exit(0);
}
