CC = gcc
CFLAGS = -Wall -Wextra -O2

all: screamplayer

screamplayer: screamplayer.c
	$(CC) $(CFLAGS) -o screamplayer screamplayer.c

clean:
	rm -f screamplayer