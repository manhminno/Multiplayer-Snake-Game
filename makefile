# Makefile for Snake v1.0
# Luke Collins 2017
# drmenguin.com

CC=gcc
CFLAGS=-Wall
LFLAGS=-lncurses -lpthread

all: client server

server: server.c
	$(CC) $(CFLAGS) server.c -o server $(LFLAGS)

client: client.c
	$(CC) $(CFLAGS) client.c -o client $(LFLAGS)

clean: 
	rm client server