// ***************************************************************************
//  ... Copyright (c) 2012 Luminator
//  ... All rights reserved.
//  ... Any use without the prior written consent of Luminator
//      is strictly prohibited.
// ***************************************************************************
// ***************************************************************************
//
//  Filename:       sysGlobals.h
//
//  Description:    Global definitions for system ids etc
//
//  Revision History:
//  Date        Name            Ver     Remarks
//  04/02/2011  Joe Halpin      1 
//  Notes:
//
// ***************************************************************************

#ifndef SYSMESSAGES_H_INCLUDED
#define SYSMESSAGES_H_INCLUDED

#include <stdint.h>

//
// The system ID has to be shared among all processes, and must be unique
// within the set of interacting radlib systems (which so far is just us).
//

#define VMB_SYSTEM_ID 17
#define VMB_VERSION_STRING "2.0.0"

#endif
