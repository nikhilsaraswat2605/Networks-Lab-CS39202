CC = gcc
FLAGS = -Wall

all: libmsocket.a

libmsocket.a: mysocket.o
	ar rcs libmsocket.a mysocket.o

mysocket.o: mysocket.c mysocket.h
	$(CC) $(FLAGS) -c mysocket.c

clean:
	rm -f *.o *.a