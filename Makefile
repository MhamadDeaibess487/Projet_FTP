.PHONY: all clean

.SUFFIXES:

CC = gcc
CFLAGS = -Wall -Werror
LDFLAGS =
LIBS = -lpthread
INCLDIR = -I.

CLIENT_BIN = CLIENT/FTP_client
SERVER_BIN = SERVER/FTP_serverp 
MASTER_BIN = SERVER/FTP_master

all: $(CLIENT_BIN) $(SERVER_BIN) $(MASTER_BIN)

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) $(INCLDIR) -c -o $@ $<

CLIENT/FTP_client.o: CLIENT/FTP_client.c csapp.h protocol_commun.h CLIENT/FTP_client.h 
	$(CC) $(CFLAGS) $(INCLDIR) -c -o $@ $<

SERVER/FTP_serverp.o: SERVER/FTP_serverp.c csapp.h protocol_commun.h SERVER/FTP_serverp.h
	$(CC) $(CFLAGS) $(INCLDIR) -c -o $@ $<

SERVER/FTP_master.o: SERVER/FTP_master.c csapp.h protocol_commun.h SERVER/FTP_master.h
	$(CC) $(CFLAGS) $(INCLDIR) -c -o $@ $<

$(CLIENT_BIN): CLIENT/FTP_client.o csapp.o 
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

$(SERVER_BIN): SERVER/FTP_serverp.o csapp.o 
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

$(MASTER_BIN): SERVER/FTP_master.o csapp.o 
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

clean:
	rm -f csapp.o CLIENT/*.o SERVER/*.o $(CLIENT_BIN) $(SERVER_BIN) $(MASTER_BIN)