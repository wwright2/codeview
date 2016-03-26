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
# Makefile for common stuff
# 
# Joe Halpin
# 	Original
# 
###############################################################################

offset = ..

include ../make.conf

IMAGE = libctss.so
SRCS_C = log.c config.c registerWithWatchdog.c ctssTimer.c radlist.c

#SRCS_C = log.c config.c registerWithWatchdog.c
OBJS = $(SRCS_C:.c=.o)
DESTLIB := ../target/usr/lib

DEBUG += -g
INCLUDES += -I../include
CXXFLAGS += $(DEBUG) $(INCLUDES)
#CFLAGS += --std=c99 $(CXXFLAGS)
CFLAGS +=  $(CXXFLAGS) -DUSE_PTHREAD 
LIBS += -ldl 
LIBPATH += -L.

### Define the dependency builder
CREATE_DEPS := $(foreach file,$(SRCS),$(CC) -MM -MT $(addsuffix .o,$(basename $(file))) $(LIBLIST) $(CFLAGS) $(file) >> .depend;)

########################## T A R G E T   I N F O  ############################
all: $(IMAGE)

#	$(CC) $(LIBPATH) -Wl,  -L/home/luminator/host/usr/lib  -shared $(OBJS) -o $@ 
#	$(LD) $(LIBPATH) -shared $(OBJS) -o $@ 

$(IMAGE): .depend $(OBJS)
	@echo OBJS = $(OBJS)
	@echo $(CC) $(LIBPATH) -Wl, -shared $(OBJS) -o $@
	$(LD) $(LIBPATH) -shared $(OBJS) -o $@ 
	if [ ! -d ../lib ]; then mkdir ../lib; fi
	cp $@ ../lib

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o : %.c
	$(CC) $(LIBPATH) $(CXXFLAGS) -c $< -o $@

cfgTest: cfgTest.c
	$(CC) $(CFLAGS) $< -o $@ -L. -lctss -lstdc++


export: $(IMAGE)
	if [ ! -d $(DESTLIB) ]; then mkdir -p $(DESTLIB); fi
	cp $(IMAGE) $(DESTLIB)

clean:
	-rm -f *.o $(TGT) $(OBJS) 
	rm -rf $(OBJS) $(IMAGE) $(IMAGE).map .depend ../bin/$(IMAGE) cfgTest *~


#  Generate dependencies
.depend:
	@echo "Generating dependencies..."
	@$(CREATE_DEPS)

#  include the dependencies
-include .depend


