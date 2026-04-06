CC = gcc
CFLAGS = -Wall -Wextra -O2 $(shell pkg-config --cflags sndfile)
LIBS = $(shell pkg-config --libs sndfile)

all: screamplayer

screamplayer: screamplayer.c
	$(CC) $(CFLAGS) -o screamplayer screamplayer.c $(LIBS)

clean:
	rm -f screamplayer