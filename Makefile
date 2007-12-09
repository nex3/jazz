VERSION=$(shell cat VERSION)
MAJOR_VERSION=$(shell cat VERSION | sed -r 's/^([0-9]+)\.([0-9]+)\.([0-9]+)/\1/')
MINOR_VERSION=$(shell cat VERSION | sed -r 's/^([0-9]+)\.([0-9]+)\.([0-9]+)/\2/')
MINI_VERSION= $(shell cat VERSION | sed -r 's/^([0-9]+)\.([0-9]+)\.([0-9]+)/\3/')

CC= gcc
CFLAGS= -fPIC -Wall
LDFLAGS=

default: jazz

clean:
	rm -f jazz libjazz.so* *.o

jazz: main.o libjazz
	$(CC) -L. -ljazz -o $@ main.o

main.o: main.c

libjazz: hello.o
	gcc -shared -Wl,-soname,$@.so.$(MAJOR_VERSION) -o $@.so.$(VERSION) $?
	ln -s $@.so.$(VERSION) $@.so.$(MAJOR_VERSION).$(MINOR_VERSION)
	ln -s $@.so.$(MAJOR_VERSION).$(MINOR_VERSION) $@.so.$(MAJOR_VERSION)
	ln -s $@.so.$(MAJOR_VERSION) $@.so

hello.o: hello.c hello.h
