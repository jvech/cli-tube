CC = gcc
CFLAGS=-std=c99 -pedantic-errors

build: main.c
	$(CC) $(CFLAGS) main.c -o main

debug: main.c 
	$(CC) -g main.c -o main $(CFLAGS)
