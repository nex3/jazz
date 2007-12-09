CC= gcc
CFLAGS= -Wall
LDFLAGS=

default: jazz

clean:
	rm jazz *.o

jazz: hello.o
	$(CC) -o $@ $?

hello.o: hello.c
