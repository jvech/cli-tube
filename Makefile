CC = cc
CFLAGS=-std=c99 -pedantic-errors -Wall
LIBS = -lncursesw
BIN=cli-tube

all: build

build: main.c
	$(CC) $(CFLAGS) main.c -o $(BIN) $(LIBS)

debug: main.c 
	$(CC) -g main.c -o $(BIN) $(CFLAGS) $(LIBS)
	valgrind --leak-check=full ./$(BIN) foo bar

clean:
	rm $(BIN) -v

install: build
	cp $(BIN) /usr/bin/
	chmod 755 /usr/bin/$(BIN)

uninstall: 
	rm -v /usr/bin/$(BIN)
