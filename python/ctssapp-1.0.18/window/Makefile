#******************************************************************************
#*****                                                                    *****
#*****                 Copyright (c) 2008 Luminator USA                   *****
#*****                      All Rights Reserved                           *****
#*****    THIS IS UNPUBLISHED CONFIDENTIAL AND PROPRIETARY SOURCE CODE    *****
#*****                        OF Luminator USA                            *****
#*****       ANY UNAUTHORIZED USE, REPRODUCTION, OR TRANSFER OF THIS      *****
#*****                  PROGRAM IS STRICTLY PROHIBITED                    *****
#*****          The copyright notice above does not evidence any          *****
#*****         actual or intended publication of such source code         *****
#*****                                                                    *****
#******************************************************************************
###############################################################################
#
#  Name                 Date           Description
#  -------------------------------------------------------------------------  #
#  
###############################################################################

vpath %.c ../common

SRCS		:= font.c graphic.c textWindow.c \
		   winRender.c window.c windowList.c

OBJS 		:= $(SRCS:.c=.o)

# Include our general use configuration
include ../make.conf

###  NOTE:  DON'T REDEFINE THE FOLLOWING MACROS, JUST APPEND  ###
###  Most of what you need is defined in make.conf
LIBS		+= -ldl -lpthread
SYSTEMTASK	+= 
LIBPATH 	+=
DEFINES		+= -D_SVID_SOURCE
INCLUDES	+= -I../include -I../common
LDFLAGS		+= $(LIBPATH) $(LIBS) -pthread -lpng -Wno-format-extra-args
TGTS        	= $(OUTDIR)/winRender-2-2 $(OUTDIR)/winRender-5-2
CFLAGS 		= -std=c99 $(INCLUDES) $(DEFINES)
CPPFLAGS 	:=
CXXFLAGS 	:= -Wno-deprecated $(DEBUGFLAGS)

.PHONY : updatePkg all 

#  Default Target
all: $(OBJS)

updatePkg: 
	@echo nothing to do

clean: 
	-rm -f *.o *~
