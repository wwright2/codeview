#include ../../make.conf
#DEFINES += -DON_BOARD # define this only when running on the board
#LDFLAGS += -lefence
CC=gcc

vpath %.c ../../common ../../window ../../display
TGT = display
SRCS =  main.c font.c log.c window.c winRender.c textWindow.c graphic.c \
	sharedData.c

OBJS = $(SRCS:.c=.o)
DEFINES += -D_SVID_SOURCE -DUSE_PTHREAD 
#DEFINES += -D_SVID_SOURCE -DPIXEL_3 
CFLAGS  = -I. -I../../window -I../../include --std=gnu99 --std=c99 \
	  -g -ggdb -I../common -Wall -pthread $(DEFINES) \
	  -I../../display
LDFLAGS += -L../lib -lstdc++ -lpthread -lncurses -lpng -ldl

all: $(TGT)

test: test.o
	$(CC)  -o $@ $< $(CFLAGS) $(LDFLAGS) $(LIBS)

bcast: bcast.o
	$(CC)  -o $@ $< $(CFLAGS) $(LDFLAGS) $(LIBS)

$(TGT): $(OBJS)
	$(CC)  -o $@ $(OBJS) $(CFLAGS) $(LDFLAGS) $(LIBS)
	@echo "$@ build complete."

clean:
	rm -rf $(OBJS) $(TGT) .depend ../bin/$(TGT) *.map *~

