VERSION=$(shell cat VERSION)
MAJOR_VERSION=$(shell cat VERSION | sed -r 's/^([0-9]+)\.([0-9]+)\.([0-9]+)/\1/')
MINOR_VERSION=$(shell cat VERSION | sed -r 's/^([0-9]+)\.([0-9]+)\.([0-9]+)/\2/')
MINI_VERSION= $(shell cat VERSION | sed -r 's/^([0-9]+)\.([0-9]+)\.([0-9]+)/\3/')

CC= gcc
CFLAGS= -g -fPIC -ansi -Wall -pedantic -DJZ_DEBUG_LEX=0 -DJZ_DEBUG_BYTECODE=0
LDFLAGS=
INCLUDES= -licui18n -licuio

YACC= bison
YFLAGS= -y-d

default: jazz

clean:
	rm -f jazz libjazz.so* src/keywords.gp.c src/y.tab.* src/*.o

jazz: src/main.o libjazz
	$(CC) -L. -ljazz -o $@ src/main.o

main.o: src/main.c src/lex.h src/string.h src/vm.h src/compile.h

libjazz: src/lex.o src/string.o src/y.tab.o src/vm.o src/compile.o src/type.o
	$(CC) -shared $(INCLUDES) -Wl,-soname,$@.so.$(MAJOR_VERSION) -o $@.so.$(VERSION) $?
	ln -sf $@.so.$(VERSION) $@.so.$(MAJOR_VERSION).$(MINOR_VERSION)
	ln -sf $@.so.$(MAJOR_VERSION).$(MINOR_VERSION) $@.so.$(MAJOR_VERSION)
	ln -sf $@.so.$(MAJOR_VERSION) $@.so

src/keywords.gp.c: src/keywords.gperf
	gperf --output-file=$@ -I -t -C -E $?

src/y.tab.o: src/y.tab.c src/y.tab.h
src/y.tab.c src/y.tab.h: src/parse.y src/parse.h src/opcode.h
	cd src && $(YACC) $(YFLAGS) parse.y

src/type.o: src/type.c src/type.h
src/compile.o: src/compile.c src/compile.h src/parse.h src/opcode.h src/type.h
src/vm.o: src/vm.c src/vm.h src/opcode.h src/type.h src/compile.h
src/lex.o: src/lex.c src/lex.h src/string.h src/y.tab.h src/keywords.gp.c
src/string.o: src/string.c src/string.h
