.SILENT:
#.SUFFIXES : .c .o

VERSION=1.0

TARGET=boot

#CROSS_MV=/usr/local/marvell/mv6281/sdk/3.2/bin/arm-mv5sft-linux-gnueabi-
#CROSS_OCTEON=/usr/local/Cavium_Networks/OCTEON-SDK-2.3/tools/bin/mips64-octeon-linux-gnu-
#CROSS_ARM=/usr/local/sdk/atmel/sam9g25/sdk/3.2/bin/arm-mv5sft-linux-gnueabi-


#ifeq ($(CROSS),)  
# $(error Makefile need Cross Compiler)  
#endif

PLATFORM=openwrt

ifeq ($(PLATFORM),arm)  
CROSS=/usr/local/sdk/atmel/sam9g25/sdk/3.2/bin/arm-mv5sft-linux-gnueabi-
else ifeq ($(PLATFORM),octeon)  
CROSS=/usr/local/Cavium_Networks/OCTEON-SDK-2.3/tools/bin/mips64-octeon-linux-gnu-
else ifeq ($(PLATFORM),marvell)  
CROSS=/usr/local/marvell/mv6281/sdk/3.2/bin/arm-mv5sft-linux-gnueabi-
else ifeq ($(PLATFORM),openwrt)  
CROSS=/home_ssd/jong2ry/VOS/m2mg30_openwrt/buildroot/staging_dir/toolchain-atmel_sam9x/bin/arm-openwrt-linux-uclibcgnueabi-
else
$(error Makefile need Cross Compiler)  
endif

BASEDIR=$(shell pwd)
SRCDIR1=$(BASEDIR)
VPATH = $(SRCDIR1)

CC=$(CROSS)gcc
CPP=$(CROSS)g++
AR=$(CROSS)ar
LD=$(CROSS)ld
STRIP=$(CROSS)strip

INCDIR=-I$(BASEDIR) -I$(BASEDIR)/include -I$(SRCDIR1) -I$(SRCDIR2)
CFLAGS=-Wno-unused-but-set-variable -g $(INCDIR) -DHAVE_LTE_INTERFACE
LDFLAGS=

SRCS = $(foreach dir, . $(SRCDIR1) , $(wildcard $(dir)/*.c))             
SRCS := $(notdir $(SRCS)) 
OBJS =$(SRCS:.c=.o)

all:$(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $^ -o $(TARGET).$(PLATFORM) -lpthread && echo '$^ -> $(TARGET).$(PLATFORM)'
	cp $(TARGET).$(PLATFORM) ~/BINARY/
	@echo 'Build Complete :  $(TARGET).$(PLATFORM)'
	@echo ''
	@echo ''

clean:
	rm -f $(OBJS)
	rm -f $(TARGET).$(PLATFORM)

distclean:
	make clean
