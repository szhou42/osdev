# Setup compiler name and flags
CC=i686-elf-gcc
CFLAGS=-Wall -Wno-int-conversion -fstrength-reduce -fomit-frame-pointer -finline-functions -nostdlib -ffreestanding -I./include -g -ggdb -c
LDFLAGS=-T link.ld -ffreestanding -O2 -nostdlib -g -ggdb

all:umain.c userentry

userentry:umain.c
	$(CC) $(CFLAGS) umain.c -o userentry
	# Write this userentry to our hard disk image, so that loader can load and execute it
	
