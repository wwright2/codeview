#include <windows.h>

/*  Declare Windows procedure  */
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);

/*  Make the class name into a global variable  */
char szClassName[ ] = "Example Program";

int WINAPI WinMain (HINSTANCE hThisInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszArgument,
                    int nFunsterStil)

{
    HWND hwnd;               /* This is the handle for our window */
    MSG messages;            /* Here messages to the application are saved */
    WNDCLASSEX wincl;        /* Data structure for the windowclass */

    /* The Window structure */
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
    wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
    wincl.cbSize = sizeof (WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;                 /* No menu */
    wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
    wincl.cbWndExtra = 0;                      /* structure or the window instance */
    /* Use Windows's default color as the background of the window */
    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx (&wincl))
        return 0;

    /* The class is registered, let's create the program*/
    hwnd = CreateWindowEx (
           0,                   /* Extended possibilites for variation */
           szClassName,         /* Classname */
           "Example Program",       /* Title Text */
           WS_OVERLAPPEDWINDOW, /* default window */
           CW_USEDEFAULT,       /* Windows decides the position */
           CW_USEDEFAULT,       /* where the window ends up on the screen */
           544,                 /* The programs width */
           375,                 /* and height in pixels */
           HWND_DESKTOP,        /* The window is a child-window to desktop */
           NULL,                /* No menu */
           hThisInstance,       /* Program Instance handler */
           NULL                 /* No Window Creation data */
           );

    /* Make the window visible on the screen */
    ShowWindow (hwnd, nFunsterStil);

    /* Run the message loop. It will run until GetMessage() returns 0 */
    while (GetMessage (&messages, NULL, 0, 0))
    {
        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);
        /* Send message to WindowProcedure */
        DispatchMessage(&messages);
    }

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}

#define ID_Save 1
#define ID_Load 2
#define ID_Exit 3
#define ID_Undo 4
#define ID_Redo 5
#define ID_VWS 6

/*  This function is called by the Windows function DispatchMessage()  */

LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)                  /* handle the messages */
    {
           case WM_CREATE:{
                HMENU hMenubar = CreateMenu();
                HMENU hFile = CreateMenu();
                HMENU hEdit = CreateMenu();
                HMENU hHelp = CreateMenu();
                
                AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hFile, "File");           
                AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hEdit, "Edit");
                AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hHelp, "Help");
                
                AppendMenu(hFile, MF_STRING, ID_Save, "Save");
                AppendMenu(hFile, MF_STRING, ID_Load, "Load");
                AppendMenu(hFile, MF_STRING, ID_Exit, "Exit");
                
                AppendMenu(hEdit, MF_STRING, ID_Undo, "Undo");
                AppendMenu(hEdit, MF_STRING, ID_Redo, "Redo");
                
                AppendMenu(hHelp, MF_STRING, ID_VWS, "Visit website");
           
           SetMenu(hwnd, hMenubar);
           break;}
           
           case WM_COMMAND:{
                if(LOWORD(wParam) == ID_Exit) {
                exit(0);
                }
                
                if(LOWORD(wParam) == ID_VWS) {
                ShellExecute(NULL, "Open", "http://www.yoursitehere.com/", NULL, NULL, SW_SHOWNORMAL);}
                
                
                
           break;}
           
        case WM_DESTROY:
            PostQuitMessage (0);       /* send a WM_QUIT to the message queue */
            break;
        default:                      /* for messages that we don't deal with */
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}
