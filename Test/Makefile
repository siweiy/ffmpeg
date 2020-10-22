
CC = gcc
CXX = g++
SRCS     := $(wildcard *.c)
OBJS     := $(patsubst %.c, %.o, $(SRCS))
TARGETS  := $(SRCS:%.c=%)

CFLAGS := -g -Wall
INCLUDE := -I$$(pwd)/../ThirdParty/ffmpeg/include \
		  -I$$(pwd)/../ThirdParty/librtmp/include \
		  -I$$(pwd)/../Sample \
		  -I$$(pwd)/../src
		  
LIBS := -L$$(pwd)/../ThirdParty/librtmp/lib

LIB_CC := -lavcodec -lavdevice -lavformat -lavutil -lavfilter \
		   -lpostproc -lswresample -lswscale -lrtmp

# $(info $(OBJS))
# $(info $(TARGETS))

all:$(TARGETS)
$(TARGETS):%:%.o
	$(CC) $^ -o $@ $(INCLUDE) $(LIBS) $(LIB_CC) $(CFLAGS)
	# @rm -rf $(OBJS) $(TARGETS)

%.o:%.c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE)

.PHONY: clean
clean:
	@rm -rf $(TARGETS) $(OBJS)