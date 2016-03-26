
#ifndef CTSS_TTS_h
#define CTSS_TTS_h 1

extern int ttsInit();
extern void ttsExit();
extern int ttsPlay(char *text);
extern int ttsStop();

#endif
