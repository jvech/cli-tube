CC = gcc
CFLAGS=-std=c99 -pedantic-errors -Wall
BIN=cli-tube

build: main.c
	$(CC) $(CFLAGS) main.c -o $(BIN)

debug: main.c 
	$(CC) -g main.c -o $(BIN) $(CFLAGS)
	valgrind --leak-check=full ./$(BIN) foo bar

clean:
	rm main *.o $(BIN) -v

install: build
	cp $(BIN) /usr/bin/
	chmod 755 /usr/bin/$(BIN)

uninstall: 
	rm /usr/bin/$(BIN)

