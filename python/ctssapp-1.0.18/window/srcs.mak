vpath 		%.c ../common
MYSRCS 		:= render.c qMessages.c windowOps.c fontOps.c windowList.c rtc.c scan.c bmp.c ledCheck.c \
               window.c font.c winRender.c graphic.c msgStrings.c textWindow.c

SRCS   		:= $(MYSRCS) $(OTHERSRCS)

MAKEFILES	:= make5-2.mak make2-2.mak make2-1.mak

### Define the object list automagically
OBJS 		:= $(SRCS:.c=.o)

CFLAGS  +=  -Wno-format-extra-args
