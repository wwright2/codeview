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
# Makefile for the CD Shelter Sign controller
# 
# Joe Halpin
# 	Original
# 
###############################################################################

include make.conf

# create ln -s ~/ctss-repo/tags/cepstral-1.0.0/uclib/cepstral_david
$(shell ./cepstral.sh)

TGT = ctss.tgz
DIRS = busapi common fonts display system gpio hwwd test/timertest test/cepstral button install


all: $(TGT)

$(TGT):
	dirs='$(DIRS)'; for d in $$dirs; do \
	    (cd $$d && $(MAKE)); \
	done

export:
	-rm -rf target >/dev/null
	mkdir -p target/ctss/run 2>/dev/null
	mkdir -p target/ctss/generatorXml 2>/dev/null
	mkdir -p target/ctss/gtfs 2>/dev/null
	mkdir -p target/ctss/cfg 2>/dev/null
	mkdir -p target/ctss/bin 2>/dev/null
	mkdir -p target/ctss/install 2>/dev/null
	mkdir -p target/ctss
	dirs='$(DIRS)'; for d in $$dirs; do \
	    (cd $$d && $(MAKE) export); \
	done
	cp busapi/generatorXml/* target/ctss/generatorXml
	cp cfg/* target/ctss/cfg
	./mkUpdatePackage.sh $(mmVersion)

clean:
	dirs='$(DIRS)'; for d in $$dirs; do \
	    (cd $$d && $(MAKE) clean); \
	done
	-rm -f *.o $(TGT) *~
	-rm -rf target lib
	-rm *.bz2
