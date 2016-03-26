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
#include host.conf

VP=../common ../window
vpath %.c $(VP)

TGT = display
MYSRCS = main.c networkThread.c scannerThread.c sharedData.c \
	apiThread.c renderThread.c arrivalFuncs.c hexdump.c \
	apiCommon.c errorFuncs.c
OTHERSRCS = log.c font.c graphic.c textWindow.c winRender.c \
	window.c windowList.c

SRCS = $(MYSRCS) $(OTHERSRCS)
OBJS = $(SRCS:.c=.o)
DEBUG += -g -DUSE_PTHREAD -O0
#DEBUG += -DDEBUG_THREAD
INCLUDES += -I../include -I../common -I../window 
CFLAGS += --std=gnu99 $(DEBUG) $(INCLUDES) -Wall -pthread
LDFLAGS += $(LIBPATH) -ldl -L../lib -lstdc++ -lpng -lpthread 
BIN = ../target/ctss/bin 
CREATE_DEPS := ../createDeps

all: $(TGT)

$(TGT): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	-mkdir -p $(BIN)
	cp $(TGT) $(BIN)
	@echo "$@ build complete."

export: $(TGT)
	if [ ! -d $(BIN) ]; then mkdir -p $(BIN); fi
	cp $(TGT) $(BIN)
	cp dispTest.sh msgs.txt $(BIN)
	chmod -R 750 $(BIN)

clean:
	rm -rf $(OBJS) $(TGT) .depend ../bin/$(TGT) *.map *~

#  Generate dependencies
.depend:
	@echo "Generating dependencies..."
	@$(CREATE_DEPS "$(SRCS)" "$(CC)" "$(CFLAGS)" "$(VP)")

#  include the dependencies
-include .depend

