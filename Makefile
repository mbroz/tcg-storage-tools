#CPPFLAGS=
#CFLAGS=-O0 -g -Wall
#LDLIBS=
#CC=gcc

#SOURCES=$(wildcard *.c)
#OBJECTS=$(SOURCES:.c=.o)

all: discovery

discovery: discovery.o
	$(CC) -o $@ $^ $(LDLIBS)

test: test.o
	$(CC) -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o *~ core test discovery

.PHONY: clean
