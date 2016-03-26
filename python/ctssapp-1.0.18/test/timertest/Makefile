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

OFFSET	    =../..
include $(OFFSET)/make.conf

BIN	            =$(OFFSET)/target/ctss/bin
TGTS                =$(IMAGE) 

SRCS = 	ttest.c \


# OBJS = $(SRCS:.c=.o)
OBJS := $(addsuffix .o,$(basename $(SRCS)))

INCLUDES    += \
			-I$(OFFSET)/common

CFLAGS += -g $(INCLUDES) -std=c99 -D_BSD_SOURCE


###  NOTE:  DON'T REDEFINE THE FOLLOWING MACROS, JUST APPEND  ###
#  Libraries
LIBS	+= -lpthread -lctss 

LIBPATH	+= -L$(OFFSET)/lib


### Define the dependency builder
CREATE_DEPS := $(foreach file,$(SRCS),$(CC) -MM -MT $(addsuffix .o,$(basename $(file))) $(CFLAGS) $(file) >> .depend;)


########################## T A R G E T   I N F O  ############################

IMAGE	= ttest

##  Default Target
all: $(IMAGE)

$(IMAGE): .depend $(OBJS)
	-mkdir -p $(BIN)
	@echo "Linking $@..."
	$(CC) $(LIBPATH)  -o $@  $(OBJS)  $(LIBS)
	@echo "$@ build complete."

#	python -m compileall -lf "."


clean:
	rm -rf $(OBJS) $(IMAGE) $(IMAGE).map .depend ../bin/$(IMAGE) *.pyc


export:
ifeq ($(TEST),test)
	-mkdir -p $(BIN)
	@cp $(IMAGE) $(BIN)
endif

#  Generate dependencies
.depend:
	@echo "Generating dependencies..."
	@$(CREATE_DEPS)


#  include the dependencies
-include .depend



