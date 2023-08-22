# Makefile for client and server

CC = gcc
OBJCS = drone8.c
FUNCT = functions.c

CFLAGS =  -g -Wall
# setup for system
LIBS = -lm

all: drone8

drone8: $(OBJCS)
	$(CC) $(CFLAGS) -o $@ $(OBJCS) $(LIBS) $(FUNCT)

functions: $(FUNCT)
	$(CC) $(CFLAGS) -o $@ $(LIBS) $(FUNCT)


clean:
	rm drone8