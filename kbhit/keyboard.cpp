#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "keyboard.h"

/**********************************************************\
    Constructor
    Save current stdin state
    Configure terminal IO for none blocking call
\**********************************************************/
Keyboard::Keyboard() :
    m_bVirtualKey(false),
    m_bPgUp(false),
    m_bPgDn(false),
    m_oldf(0),
    m_peek_character(-1)
{
    tcgetattr(STDIN_FILENO, &m_oldt);
    m_newt = m_oldt;
    m_newt.c_lflag &= ~ICANON;
    m_newt.c_lflag &= ~ECHO;
    m_newt.c_lflag &= ~ISIG;
    m_newt.c_cc[VMIN] = 1;
    m_newt.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &m_newt);
}

/**********************************************************\
    Destructor
    Restore original stdin state
\**********************************************************/
Keyboard::~Keyboard()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &m_oldt);
}


/**********************************************************\
    kbhit
    return 0 if not character available
    return 1 if one character is present
    To not consume too much CPU we sleep for 10 ms
    when no character is present.
\**********************************************************/
int Keyboard::kbhit()
{
    int iRdCnt = 0;
    if (m_peek_character == -1) {
        m_newt.c_cc[VMIN] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &m_newt);

        unsigned char ch;
        iRdCnt = read(STDIN_FILENO, &ch, 1);
        m_newt.c_cc[VMIN] = 1;
        tcsetattr(STDIN_FILENO, TCSANOW, &m_newt);

        if (iRdCnt == 1) {
            m_peek_character = ch;
        } else {
            // Spare CPU when no activity is detected on the keyboard
            usleep(10*1000);
        }
    }
    return iRdCnt;
}

/**********************************************************\
    getch
    return last peek character is exists otherwise 
    read from the console
\**********************************************************/
int Keyboard::getch()
{
    char ch = -1;

    if (m_peek_character != -1) {
        ch = m_peek_character;
        m_peek_character = -1;
    } else {
        read(STDIN_FILENO, &ch, 1);        
    }

    return ch;
}

/**********************************************************\
    OnKeyPad
    is a helper method to do the last step of key pad convertion
\**********************************************************/
int Keyboard::OnKeyPad(int ch, eVirtualKey vkey)
{
    int key = ch;
    if(m_bVirtualKey) {
        m_bVirtualKey = false;
        key = (int)vkey;
    }
    return key;
}

/**********************************************************\
    OnKey5ch
    helper method when key 5 is pressed
\**********************************************************/
int Keyboard::OnKey5ch(int ch, eVirtualKey vkey)
{
    int key = ch;
    if(m_bVirtualKey) {
        m_bPgUp = true;
    }
    return key;
}

/**********************************************************\
    OnKey6ch
    helper method when key 6 is pressed
\**********************************************************/
int Keyboard::OnKey6ch(int ch, eVirtualKey vkey)
{
    int key = ch;
    if(m_bVirtualKey) {
        m_bPgDn = true;
    }
    return key;
}

/**********************************************************\
    OnKeyTilde
    helper method when key ~ is pressed
\**********************************************************/
int Keyboard::OnKeyTilde(int ch, eVirtualKey vkey)
{
    int key = ch;
    if(m_bVirtualKey) {
        if(m_bPgUp) {
            ch = (int)eKeyPageUp;
        }
        if(m_bPgDn) {
            ch = (int)eKeyPageDown;
        }
        m_bVirtualKey = false;
        m_bPgDn = false;
        m_bPgUp = false;
    }
//    fprintf(stdout, "key: %d\n", key);
    return key;
}

/**********************************************************\
    OnKey27dec
    helper method when key 27 (dec) is pressed
\**********************************************************/
int Keyboard::OnKey27dec(int ch, eVirtualKey vkey)
{
    int key = ch;
    m_bVirtualKey = true;   // Starting virtual key sequence
//    fprintf(stdout, "27dec key: %d\n", ch);
    return key;
}

/**********************************************************\
    OnKey91dec
    helper method when key 91 (dec) is pressed
\**********************************************************/
int Keyboard::OnKey91dec(int ch, eVirtualKey vkey)
{
    int key = ch;
    // middle key for num pad. Do nothing
//    fprintf(stdout, "91dec key: %d\n", ch);
    return key;
}

/**********************************************************\
    Transcode virtual key for pad. Do not change ASCII char
    arrow up    => eKeyArrowUp
    arrow down  => eKeyArrowDown
    arrow left  => eKeyArrowLeft
    arrow right => eKeyArrowRight
    Page Up     => eKeyPageUp
    Page Down   => eKeyPageDown
    Home        => eKeyHome
    End         => eKeyEnd
\**********************************************************/
int Keyboard::TranscodeVirtualKey(int ch)
{
    #define ON(k,f,vk) case k: ch = f(ch, vk); break;
    switch(ch) {
        ON('A', Keyboard::OnKeyPad,    eKeyArrowUp);
        ON('B', Keyboard::OnKeyPad,   eKeyArrowDown);
        ON('C', Keyboard::OnKeyPad,   eKeyArrowRight);
        ON('D', Keyboard::OnKeyPad,   eKeyArrowLeft);
        ON('F', Keyboard::OnKeyPad,   eKeyEnd);
        ON('H', Keyboard::OnKeyPad,   eKeyHome);
        ON('5', Keyboard::OnKey5ch,   eKeyNone);
        ON('6', Keyboard::OnKey6ch,   eKeyNone);
        ON('~', Keyboard::OnKeyTilde, eKeyNone);
        ON(27,  Keyboard::OnKey27dec, eKeyNone);
        ON(91,  Keyboard::OnKey91dec, eKeyNone);
        default:
//            fprintf(stdout, "default key: %d\n", ch);
            break;
    }
    #undef ON
    return ch;
}

