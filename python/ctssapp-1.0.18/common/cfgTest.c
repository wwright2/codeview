#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "common.h"

void die (const char *msg)
{
    perror (msg);
    exit (1);
}


int main()
{
    if (cfgOpen ("test.cfg") == -1)
        die ("Error opening config file");

    printf ("get key1: %s\n", cfgGet ("key1"));
    printf ("get key2: %s\n", cfgGet ("key2"));

    printf ("Setting key1 = 'value one'\n");
    cfgSet ("key1", "value one");
    
    printf ("Setting key2 = 'value two'\n");
    cfgSet ("key2", "value two");

    cfgClose ();
}

    
