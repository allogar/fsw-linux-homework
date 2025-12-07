# Makefile

CC = gcc
CFLAGS = -std=c11 -Wall -O2 -pthread

all: client1

client1: client1.c
	$(CC) $(CFLAGS) -o client1 client1.c

clean:
	rm -f client1