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
TGT = display
SRCS =  main.c display.c network.c font.c apiMsg.c render.c alerts.c log.c \
	errors.c arrivals.c ctaConfig.c

OBJS = $(SRCS:.c=.o)
DEBUG = -g #-DTHR_DEBUG
CFLAGS += -I --std=gnu99 --std=c99 $(DEBUG) -I../common -Wall -pthread -DUSE_PTHREAD
LDFLAGS = -ldl -L../lib -lstdc++ -lpthread -Wl,-Map,display.map -lncurses
BIN = ../target/ctss/bin 
CREATE_DEPS := $(foreach file,$(SRCS),$(CC) -MM -MT $(addsuffix .o,$(basename $(file))) $(CFLAGS) $(file) >>                        .depend;)

all: $(TGT)

$(TGT): .depend $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
	-mkdir -p $(BIN)
	cp $(TGT) $(BIN)
	@echo "$@ build complete."

export: $(TGT)
	if [ ! -d $(BIN) ]; then mkdir -p $(BIN); fi
	cp $(TGT) $(BIN)
	chmod -R 750 $(BIN)

clean:
	rm -rf $(OBJS) $(TGT) .depend ../bin/$(TGT) *.map *~

#  Generate dependencies
.depend:
	@echo "Generating dependencies..."
	@$(CREATE_DEPS)

#  include the dependencies
-include .depend

