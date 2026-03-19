.PHONY: all clean

.SUFFIXES:

CC = gcc
CFLAGS = -Wall -Werror
LDFLAGS =
LIBS = -lpthread
INCLDIR = -I.

CLIENT_BIN = CLIENT/FTP_client
SERVER_BIN = SERVER/FTP_serverp

all: $(CLIENT_BIN) $(SERVER_BIN)

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) $(INCLDIR) -c -o $@ $<

CLIENT/FTP_client.o: CLIENT/FTP_client.c csapp.h CLIENT/FTP_client.h
	$(CC) $(CFLAGS) $(INCLDIR) -c -o $@ $<

SERVER/FTP_serverp.o: SERVER/FTP_serverp.c csapp.h SERVER/FTP_serverp.h
	$(CC) $(CFLAGS) $(INCLDIR) -c -o $@ $<

$(CLIENT_BIN): CLIENT/FTP_client.o csapp.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

$(SERVER_BIN): SERVER/FTP_serverp.o csapp.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

clean:
	rm -f csapp.o CLIENT/*.o SERVER/*.o $(CLIENT_BIN) $(SERVER_BIN)