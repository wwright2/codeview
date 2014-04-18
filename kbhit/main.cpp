#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include "keyboard.h"

/**********************************************************\
    Handler of the switch statement in the main function
    - OnKeyHandler
    - OnExit  
\**********************************************************/
bool OnKeyHandler(const char* str, int ch)
{
    printf("%s: (%d, %X) '%c'\n", str, ch, ch, ch);
    return true;
}

bool OnExit(const char* str, int ch)
{
    printf("%s: (%d, %X) '%c'\n", str, ch, ch, ch);
    return false;
}

bool OnHelp(const char* str, int ch)
{
    printf("=== List options available ===\n");
    printf("a          : Show key pressed\n");
    printf("B          : Show key pressed\n");
    printf("h          : Quick help\n");
    printf("x          : Exit\n");
    printf("Arrow up   : virtual key\n");
    printf("Arrow down : virtual key\n");
    printf("Arrow left : virtual key\n");
    printf("Arrow right: virtual key\n");
    printf("Page up    : virtual key\n");
    printf("Page down  : virtual key\n");
    printf("Home       : virtual key\n");
    printf("End        : virtual key\n");
    printf("-          : Minux\n");
    printf("+          : Plus\n");
    printf("Anything else ignored\n");
    return true;
}

/**********************************************************\
    main
    Simple application to demonstrate Keyboard class
    Functionality similart to kbhit() function on Windows
    getch() return last character and Transcode Virtual key
    if desired.
\**********************************************************/
int main(int argc, char* argv[])
{
    bool bRun = true;
    do {
        Keyboard kb;
        while(kb.kbhit()) {
            int ch = kb.getch();
            ch = kb.TranscodeVirtualKey(ch);
            #define ON(k,f,s) case k: bRun = f(s,ch); break
            switch(ch) {
                ON('a',             OnKeyHandler,   "Key");
                ON('B',             OnKeyHandler,   "Key");
                ON('h',             OnHelp,         "Help");
                ON('x',             OnExit,         "Exit");
                ON(eKeyArrowUp,     OnKeyHandler,   "Arrow up");
                ON(eKeyArrowDown,   OnKeyHandler,   "Arrow down");
                ON(eKeyArrowLeft,   OnKeyHandler,   "Arrow left");
                ON(eKeyArrowRight,  OnKeyHandler,   "Arrow right");
                ON(eKeyPageUp,      OnKeyHandler,   "Page up");
                ON(eKeyPageDown,    OnKeyHandler,   "Page down");
                ON(eKeyHome,        OnKeyHandler,   "Home");
                ON(eKeyEnd,         OnKeyHandler,   "End");
                ON('-',             OnKeyHandler,   "Minus");
                ON('+',             OnKeyHandler,   "Plus");
                default:
//                  printf("%s: (%d, %X) '%c'\n", "Ignore", ch, ch, ch);
                    break;
            }
            #undef ON
        }
    } while(bRun);
    return 0;
}
