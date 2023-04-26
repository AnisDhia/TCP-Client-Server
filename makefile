CC=gcc
CFLAGS=-Wall

all: server client

server: src/server.c
	$(CC) $(CFLAGS) -o bin/server src/server.c

client: src/client.c
	$(CC) $(CFLAGS) -o bin/client src/client.c

clean:
	rm -f server client
