CC = gcc
CFLAGS=-std=c99 -pedantic-errors -Wall
BIN=cli-tube

build: main.c
	$(CC) $(CFLAGS) main.c -o $(BIN)

debug: main.c 
	$(CC) -g main.c -o $(BIN) $(CFLAGS)
	valgrind --leak-check=full ./$(BIN) foo bar

test: test.c
	$(CC) test.c -o test $(CFLAGS) -lcurl

clean:
	rm main *.o $(BIN) -v
