CC = gcc
CFLAGS = -Wall -Wextra -O2 $(shell pkg-config --cflags sndfile samplerate)
LIBS = $(shell pkg-config --libs sndfile samplerate)

all: screamplay

screamplay: screamplay.c
	$(CC) $(CFLAGS) -o screamplay screamplay.c $(LIBS)

clean:
	rm -f screamplay