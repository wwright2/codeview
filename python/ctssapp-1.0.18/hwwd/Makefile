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
# Makefile for CTSS 
#    busapi
# 
# 
###############################################################################
# toolchain locations

# Include our general use configuration
include ../make.conf

BIN	                =../target/ctss/bin
TGTS                    =$(IMAGE)

SRCS = watchdog.c
#SRCS = setgpio.c
#SRCS = it8718_DIO.c


# OBJS = $(SRCS:.c=.o)
OBJS := $(addsuffix .o,$(basename $(SRCS)))

CFLAGS += -g -I. -std=c99


###  NOTE:  DON'T REDEFINE THE FOLLOWING MACROS, JUST APPEND  ###
#  Libraries
LIBS	+= 

LIBPATH	+=


### Define the dependency builder
CREATE_DEPS := $(foreach file,$(SRCS),$(CC) -MM -MT $(addsuffix .o,$(basename $(file))) $(CFLAGS) $(file) >> .depend;)


########################## T A R G E T   I N F O  ############################

IMAGE	= watchdog
#IMAGE	= it8718_DIO

##  Default Target
all: $(IMAGE)

$(IMAGE): .depend $(OBJS)
	-mkdir -p $(BIN)
	@echo "Linking $@..."
	$(CC) $(LIBPATH) -o $@  $(OBJS)  $(LIBS)
	@echo "$@ build complete."

clean:
	rm -rf $(OBJS) $(IMAGE) $(IMAGE).map .depend ../bin/$(IMAGE) *.pyc


export:
	@echo "Exporting $(IMAGE) ..."
	@mkdir -p $(BIN)
	@cp $(IMAGE) $(BIN)

#  Generate dependencies
.depend:
	@echo "Generating dependencies..."
	@$(CREATE_DEPS)


#  include the dependencies
-include .depend
