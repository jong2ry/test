.SILENT:

VERSION=1.0

TARGET=aut_$(VERSION)

#CROSS=/usr/local/marvell/mv6281/sdk/3.2/bin/arm-mv5sft-linux-gnueabi-
#CROSS=/usr/local/Cavium_Networks/OCTEON-SDK-2.3/tools/bin/mips64-octeon-linux-gnu-

CC=$(CROSS)gcc
CPP=$(CROSS)g++
AR=$(CROSS)ar
LD=$(CROSS)ld
STRIP=$(CROSS)strip

CFLAGS=-Wall -g
LDFLAGS=

SRCS = $(foreach dir, . $(SRCDIR1) $(SRCDIR2), $(wildcard $(dir)/*.c))             
SRCS := $(notdir $(SRCS)) 
OBJS =$(SRCS:.c=.o)

all:$(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $^ -o $(TARGET) && echo '$^ : $(TARGET)'
	cp $(TARGET) ~/BIN_UTIL/
	echo 'Build Complete $(TARGET)'
	echo ''
	echo ''

clean:
	rm -f $(OBJS)
	rm -f $(TARGET)

distclean:
	make clean
