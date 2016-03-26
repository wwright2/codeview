#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

const char *src = "The pump don't work cause the vandals took the handle";
char dst[40];
int state = -1;
int srcStart;
int srcEnd;
int srcLen;
int dstStart;
int dstEnd;
int dstLen;

void reset ()
{
    state = -1;
    srcStart = 0;
    srcEnd   = 0;
    srcLen   = strlen (src) - 1;
    dstStart = sizeof dst - 1;
    dstEnd   = sizeof dst - 1;
    dstLen   = sizeof dst;
    memset (dst, ' ', sizeof dst);
}

void waitasec ()
{
    usleep (90000);
}

int main(int arc, char *argv[])
{
    int s;
    int d;
    int i;

    if (arc > 1)
        src = argv[1];

    reset ();
    state = 0;

    fprintf (stderr, "srcLen = %d\n", strlen (src) - 1);
    fprintf (stderr, "dstLen = %d\n", dstLen);
    for (;;)
    {
        switch (state)
        {
        case -1:
            reset ();
            state = 0;
            break;

        case 0:
            if ((srcStart == srcEnd) && (dstStart == 0))
            {
                state = -1;
                break;
            }
            
            else
            {
                d = dstStart;
                s = srcStart;

                for (i = 0; i < dstEnd; ++i)
                {
                    if ((i >= dstStart) && (s <= srcEnd))
                        dst[i] = src[s++];
                    else
                        dst[i] = ' ';
                }

                if (srcLen < dstLen)
                {
                    if (srcEnd > dstEnd)
                    {
                        state = 1;
                        break;
                    }

                    if (srcEnd < srcLen)
                        srcEnd++;

                    if ((dstStart == 0) && (srcStart < srcEnd))
                        ++srcStart;
                }

                else if (srcLen >= dstLen)
                {
                    if (srcEnd > dstEnd)
                    {
                        state = 1;
                        break;
                    }
                    
                    else
                        ++srcEnd;
                }

                if (dstStart > 0)
                    dstStart--; 
            }

            dst[dstEnd] = 0;
            fprintf (stderr, "[%s]\r", dst); fflush(stderr);
            waitasec();
            break;


        case 1:
            dstStart = 0;
            ++srcStart;

            s = srcStart;
            srcLen = strlen (src) - 1;
            for (i = 0; i < (sizeof dst) - 1; ++i)
            {
                dst[i] = (s > srcLen) ? ' ' : src[s++];
            }

            dst[dstEnd] = 0;
            fprintf (stderr, "[%s]\r", dst); fflush(stderr);
            waitasec ();
            
            if (srcStart == srcLen)
                state = -1;
            break;
        }
    }
}
