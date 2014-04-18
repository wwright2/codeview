// Place virtual key code we translate above
// extended ASCII characters.
enum eVirtualKey {
	eKeyArrowUp      = 256,
	eKeyArrowDown,
	eKeyArrowLeft,
	eKeyArrowRight,
	eKeyPageUp,
	eKeyPageDown,
	eKeyHome,
	eKeyEnd,
    eKeyNone
};

class Keyboard {
public:
    Keyboard();
    virtual ~Keyboard();

    int kbhit();
    int getch();

    /**********************************************************\
        Transcode virtual key for pad. Do not change ASCII char
        arrow up    => ciKeyArrowUp
        arrow down  => ciKeyArrowDown
        arrow left  => ciKeyArrowLeft
        arrow right => ciKeyArrowRight
        Page Up     => ciKeyPageUp
        Page Down   => ciKeyPageDown
        Home        => ciKeyHome
        End         => ciKeyEnd
    \**********************************************************/
    int TranscodeVirtualKey(int ch);

protected:
    int OnKeyPad(int ch, eVirtualKey vkey);
    int OnKey5ch(int ch, eVirtualKey vkey);
    int OnKey6ch(int ch, eVirtualKey vkey);
    int OnKeyTilde(int ch, eVirtualKey vkey);
    int OnKey27dec(int ch, eVirtualKey vkey);
    int OnKey91dec(int ch, eVirtualKey vkey);

    bool m_bVirtualKey;
    bool m_bPgUp;
    bool m_bPgDn;
    int  m_oldf;
    int  m_peek_character;
    struct termios m_oldt;
    struct termios m_newt;
};
