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
TGTS                =busapi.py generatorBusapi.py

SRCS = 	 \
		

OBJS := $(addsuffix .o,$(basename $(SRCS)))
INCLUDES    += \
			
			
			

CFLAGS += 



###  NOTE:  DON'T REDEFINE THE FOLLOWING MACROS, JUST APPEND  ###
#  Libraries
LIBS	+= 

LIBPATH	+= 


### Define the dependency builder
CREATE_DEPS := $(foreach file,$(SRCS),$(CC) -MM -MT $(addsuffix .o,$(basename $(file))) $(CFLAGS) $(file) >> .depend;)


########################## T A R G E T   I N F O  ############################

IMAGE	= 

##  Default Target
all: 

#	$(LD) -static $(LIBPATH) -o $@  $(OBJS) $(LIBS)


clean:
	rm -rf $(OBJS) $(IMAGE) $(IMAGE).map .depend ../bin/$(IMAGE) *.pyc


export:
	-mkdir -p $(BIN)
	cp *.py $(BIN)

#  Generate dependencies
.depend:
	@echo "Generating dependencies..."
	@$(CREATE_DEPS)


#  include the dependencies
-include .depend



