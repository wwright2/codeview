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
# Makefile for the installation script stuff
# 
# Joe Halpin
# 	Original
# 
###############################################################################

include ../make.conf

INSTALL=../target/ctss/install
BIN=../target/ctss/bin

all: 

export:
	if [ ! -d $(BIN) ]; then mkdir -p $(BIN); fi
	if [ ! -d $(INSTALL) ]; then mkdir -p $(INSTALL); fi
	./copyFiles.sh $(BUILDROOT_VER)

clean:
	-rm *~
