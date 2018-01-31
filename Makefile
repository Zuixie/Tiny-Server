CC = gcc
CGIDIR = ./cgi-bin

all: tiny 

tiny: tiny.o csapp.o
	$(CC) -o tiny tiny.o csapp.o

tiny.o: tiny.c csapp.h
csapp.o: csapp.c csapp.h


add: add.o csapp.o
	$(CC) -o $(CGIDIR)/add add.o csapp.o

add.o: add.c csapp.h

postread: postread.o csapp.o
	$(CC) -o $(CGIDIR)/postread postread.o csapp.o

postadd.o: postread.c csapp.h

clean:
	rm -f *.o tiny
	rm -f $(CGIDIR)/*
