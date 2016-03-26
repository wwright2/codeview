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
# Makefile for font library
# 
# Joe Halpin
# 	Original
# 
###############################################################################

include ../make.conf

SRCS = Terminal_8.c Small_Fonts_8.c Nema_5x7.c
FNTS = $(SRCS:.c=.FNT.c)
LIBS = $(FNTS:.FNT.c=.so)
CFLAGS += --std=c99
SHELL := /bin/bash
LIB = ../target/ctss/fonts

all: $(LIBS)

%.so : %.FNT.c
	@echo '#include "'$*.c'"' >> fonts.h
	$(CC) -fPIC -c $< -o $*.o
	$(CC) -shared $*.o -o $@

Terminal_8.FNT.c: Terminal_8.c
	./mkFntSrc.sh $< > $@

Small_Fonts_8.FNT.c: Small_Fonts_8.c
	./mkFntSrc.sh $< > $@

Nema_5x7.FNT.c: Nema_5x7.c
	./mkFntSrc.sh $< > $@

export: 
	-mkdir -p $(LIB)
	cp *.so $(LIB)

.PHONY : clean
clean:
	-rm -f *.o fonts.h *.so *.map tgtFont.h fonts.h *.FNT.c
