.SILENT:

VERSION=1.0

TARGET=ping_test

#CROSS=/usr/local/marvell/mv6281/sdk/3.2/bin/arm-mv5sft-linux-gnueabi-
#CROSS=/usr/local/Cavium_Networks/OCTEON-SDK-2.3/tools/bin/mips64-octeon-linux-gnu-
CROSS=/usr/local/sdk/atmel/sam9g25/sdk/3.2/bin/arm-mv5sft-linux-gnueabi-

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
	$(CC) $^ -o $(TARGET) -lpthread && echo '$^ : $(TARGET)'
	cp $(TARGET) ~/BIN_UTIL/
	echo 'Build Complete $(TARGET)'
	echo ''
	echo ''

clean:
	rm -f $(OBJS)
	rm -f $(TARGET)

distclean:
	make clean
