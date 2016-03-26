// ***************************************************************************
//  … Copyright (c) 2011 Luminator Holding, LP
//  … All rights reserved.
//  … Any use without the prior written consent of Luminator Holding, LP
//    is strictly prohibited.
// ***************************************************************************
//
//  Filename:   trackerconf.c
//
//  Description:
//
//  Revision History:
//  Date             Name              Ver    Remarks
//  apr 15, 2011     wwright            0
//
//  Notes:       ...
//      process a config file
//      %s %s\n
//
// ***************************************************************************

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


#include <../common/trackerconf.h>

struct ConfigData
{
    char    key[50];
    char    value[200];
};

struct ConfigData       *config;

int     configcount;      // current entries
int     configsize;       // max entries



int  getconfigNdx(char const *key)
{
    char * val = NULL;


    for (int n=0 ; n< configcount; n++)
    {
        if (strncmp(config[n].key, key, sizeof(config[n].key)) == 0)
        {
            return n;
        }
    }
    return -1;
}

int  putconfig(char const *key, char const *value)
{
    int ndx,count;

    //debug(2,"putconfig(%s,%s), %d,%d \n", key,value,configcount,configsize);
    count = configcount;
    ndx=getconfigNdx(key);
    //debug (2,"configNdx(key)=%d\n",ndx);
    if ( ndx == -1 )
    {
        // Add
        if (configcount < configsize)
        {
            strncpy(config[count].key, key, sizeof(config[count].key) );
            strncpy(config[count].value, value, sizeof(config[count].value) );
            configcount++;
        }
        else
        {
            //Need to increase configArray.
            // ..malloc new config + size
            // ..copy old data
            // ..free old config

            return(-1);  //Return Error FULL for now.
        }
    }
    else
    {
        strncpy(config[ndx].key, key, sizeof(config[ndx].key) );
        strncpy(config[ndx].value, value, sizeof(config[ndx].value) );
    }

    return(0);

}

char *  getconfig(char const *key)
{
    int ndx;
    ndx = getconfigNdx(key);
    if ( ndx >= 0)
        return config[ndx].value;
    else
        return NULL;
}

void dumpconfig()
{
    debug (1,"dumpconfig()\n");

    for (int i=0; i< configcount; i++)
    {
        debug (1,"%s : %s\n",config[i].key, config[i].value);
    }
}

int configinit()
{
    config = (struct ConfigData *) malloc (sizeof(struct ConfigData)*DEFAULT_CONF_SIZE);
    configsize = DEFAULT_CONF_SIZE;
    configcount = 0;
}
