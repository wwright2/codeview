#******************************************************************************
#*****                                                                    *****
#*****                 Copyright (c) 2011 Luminator USA                   *****
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
# Makefile for prototype two of CTSS sign
# 
# Joe Halpin
# 	Original
# 
###############################################################################

include ../make.conf

TGT = demoDisplay
SRCS = main.c display.c font.c network.c
OBJS = $(SRCS:.c=.o)
DEBUG = -g
BIN = ../target/ctss/bin

LIBPATH +=	-L../common \
			-L.
			
LIBS    +=	-lctss \
			-ldl \
			-lpthread \
			-lstdc++ \

INCLUDES +=  -I../common
			
CFLAGS += --std=gnu99 $(DEBUG) $(INCLUDES) -Wall
LDFLAGS = $(LIBPATH) $(LIBS) -Wl,-rpath,/ctss/lib

all: $(TGT)
	$(CC) -o $(TGT) $(OBJS) $(LDFLAGS) $(CFLAGS)

$(TGT): $(OBJS) 

main.o: main.c display.c font.c mainInit.h display.h font.h

export: $(TGT)
	if [ ! -d $(BIN) ]; then mkdir -p $(BIN); fi
	cp $(TGT) $(BIN)

clean:
	-rm -f *.o $(TGT) *~