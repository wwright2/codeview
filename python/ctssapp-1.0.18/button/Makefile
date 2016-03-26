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

OFFSET = ..
# Include our general use configuration
include $(OFFSET)/make.conf

BIN	                =../target/ctss/bin
TGTS                    =$(IMAGE)

SRCS = 	buttonmon.c \
		trackerconf.c \
		tts.c


# OBJS = $(SRCS:.c=.o)
OBJS := $(addsuffix .o,$(basename $(SRCS)))
INCLUDES    += \
			-I$(OFFSET)/common \
			-I$(OFFSET)/$(CEPSTRAL_PATH)/include \
			

CFLAGS += -g $(INCLUDES) -std=c99 -D_BSD_SOURCE



###  NOTE:  DON'T REDEFINE THE FOLLOWING MACROS, JUST APPEND  ###
#  Libraries
LIBS	+= -lpthread -lctss -lswift -lstdc++ -lgcc_s

LIBPATH	+= -L../lib -L$(OFFSET)/$(CEPSTRAL_PATH)/lib


### Define the dependency builder
CREATE_DEPS := $(foreach file,$(SRCS),$(CC) -MM -MT $(addsuffix .o,$(basename $(file))) $(CFLAGS) $(file) >> .depend;)


########################## T A R G E T   I N F O  ############################

IMAGE	= buttonmon

##  Default Target
all: $(IMAGE)

#	$(LD) -static $(LIBPATH) -o $@  $(OBJS) $(LIBS)

$(IMAGE): .depend $(OBJS)
	-mkdir -p $(BIN)
	@echo "Linking $@..."
	$(CC) $(LIBPATH)  -o $@  $(OBJS)  $(LIBS)
	@echo "$@ build complete."


clean:
	rm -rf $(OBJS) $(IMAGE) $(IMAGE).map .depend  


export:
	-mkdir -p $(BIN)
	cp $(TGTS) $(BIN)


#	@cp $(IMAGE) $(BIN)

#  Generate dependencies
.depend:
	@echo "Generating dependencies..."
	@$(CREATE_DEPS)


#  include the dependencies
-include .depend



