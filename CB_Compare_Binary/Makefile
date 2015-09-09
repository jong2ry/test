.SILENT:

#CROSS=/usr/local/marvell/mv6281/sdk/3.2/bin/arm-mv5sft-linux-gnueabi-
CROSS=/usr/local/Cavium_Networks/OCTEON-SDK-2.3/tools/bin/mips64-octeon-linux-gnu-

CC=$(CROSS)gcc
CPP=$(CROSS)g++
AR=$(CROSS)ar
LD=$(CROSS)ld
STRIP=$(CROSS)strip

CFLAGS=-Wall -g
LDFLAGS=

OBJS=cb\

all:$(OBJS)

%:%.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
	cp cb ~/BIN_UTIL/

clean:
	rm -f $(OBJS)

distclean:
	make clean
