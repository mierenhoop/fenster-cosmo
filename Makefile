CC=~/projects/cosmopolitan/cosmocc/bin/cosmocc
AR=~/projects/cosmopolitan/cosmocc/bin/cosmoar

CFLAGS= -Wall -I. -I xcb/ xcb/libxcb.a

all: minimal-c.exe

xcb/libxcb.a:
	COSMOAR=$(AR) COSMOCC=$(CC) make -C xcb libxcb.a

minimal-c.exe: examples/minimal-c/main.c fenster.h xcb/libxcb.a
	$(CC) -o $@ examples/minimal-c/main.c $(CFLAGS)

DOOM_SRCS=$(wildcard examples/doom-c/*.c)

doom.exe: $(DOOM_SRCS) fenster.h xcb/libxcb.a
	$(CC) -o $@ $(DOOM_SRCS) $(CFLAGS)

clean:
	rm -f doom* minimal-c*
	make -C xcb clean
