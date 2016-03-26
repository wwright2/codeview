/******************************************************************************
 *****                                                                    *****
 *****                 Copyright (c) 2011 Luminator USA                   *****
 *****                      All Rights Reserved                           *****
 *****    THIS IS UNPUBLISHED CONFIDENTIAL AND PROPRIETARY SOURCE CODE    *****
 *****                        OF Luminator USA                            *****
 *****       ANY UNAUTHORIZED USE, REPRODUCTION, OR TRANSFER OF THIS      *****
 *****                  PROGRAM IS STRICTLY PROHIBITED                    *****
 *****          The copyright notice above does not evidence any          *****
 *****         actual or intended publication of such source code         *****
 *****                                                                    *****
 *****************************************************************************/
// ***************************************************************************
//
//  Filename:       daemon.c
//
//  Description:    function to run a process as a daemon
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  05/09/2011  Joe Halpin      1       Original
//
//  Notes: 
// 
// ***************************************************************************

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

void becomeDaemon ()
{
    int fd = open ("/dev/null", O_RDWR);

    setsid();

    close (0);
    close (1);
    close (2);
    
    dup2 (fd, 0);
    dup2 (fd, 1);
    dup2 (fd, 2);

    close (fd);
}
