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
# Makefile for system stuff
# 
# Joe Halpin
# 	Original
# 
###############################################################################

include ../make.conf

#TGT  = monitorSys.sh 
TGTS  = 
USRBIN = ../target/usr/bin
BIN  = ../target/ctss/bin
INIT = ../target/etc/init
NETWORK = ../target/etc/network
ETC  = ../target/etc
CFLAGS += -std=gnu99 $(INCLUDES) -I../common 
LDFLAGS = -ldl -L../common -L../lib -lctss



# OBJS = $(SRCS:.c=.o)
OBJS1 := $(addsuffix .o,$(basename $(SRCS1)))
OBJS2 := $(addsuffix .o,$(basename $(SRCS2)))

CFLAGS += -g -I. -std=c99

LIBPATH	+= \
	-L../common \
	-L../lib 

#  Libraries
LIBS			+= 

### Define the dependency builder
CREATE_DEPS := $(foreach file,$(SRCS),$(CC) -MM -MT $(addsuffix .o,$(basename $(file))) $(CFLAGS) $(file) >> .depend;)

##  Default Target
all: fileWatcher

$(IMAGE1): .depend $(OBJS1)
	-mkdir -p $(BIN)
	@echo "Linking $@..."
	@echo $(OBJS1)
	$(CC) $(LIBPATH) -o $@ $(LDFLAGS) $(OBJS1)  $(LIBS)
	@cp $(IMAGE1) $(BIN)
	@echo "$@ build complete."

$(IMAGE2): .depend $(OBJS2)
	@echo "Linking $@..."
	@echo $(OBJS2)
	$(CC) $(LIBPATH) -o $@ -Wl,-rpath,/ctss/lib $(OBJS2)  $(LIBS)
	@cp $(IMAGE2) $(BIN)
	@echo "$@ build complete."

#	python -m compileall -lf "."

fileWatcher: fileWatcher.o
	$(CC) $< -o $@

wdClient: wdClient.o
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ 

wdt: wdt.o
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ 

export: 
	if [ ! -d $(INIT) ]; then mkdir -p $(INIT); fi
	if [ ! -d $(ETC)  ]; then mkdir -p $(ETC); fi
	-mkdir -p $(BIN)
	-mkdir -p $(NETWORK)
	-mkdir -p ../target/bin
	-mkdir -p ../target/usr/bin
	-mkdir -p $(USRBIN)
	cp *.sh $(BIN)
	cp *.py $(BIN)
	cp *.exp $(BIN)
	cp start-ctss $(BIN)
	cp stop-ctss $(BIN)
	cp start-demo $(BIN)
	cp stop-demo $(BIN)
	cp wgwmonitor.sh $(BIN)
	cp volume.sh $(BIN)
	@echo copying fileWatcher to $(USRBIN)
	cp fileWatcher $(USRBIN)
	@echo here it is
	@ls $(USRBIN)
	chmod 750 $(BIN)/*
	chmod 750 $(USRBIN)/fileWatcher

#  Generate dependencies
.depend:
	@echo "Generating dependencies..."
	@$(CREATE_DEPS)


#  include the dependencies
-include .depend


clean:
	-rm -f *.o *~
	-rm -f ../cfg/voltest.txt ltgReboot fileWatcher
