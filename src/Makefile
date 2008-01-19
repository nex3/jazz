CC= gcc
YACC= bison -y -d
AR= ar rcu
RANLIB= ranlib

MY_CFLAGS=
CFLAGS= -g -ansi -Wall -pedantic -DJZ_DEBUG_LEX=0 -DJZ_DEBUG_BYTECODE=0 $(MY_CFLAGS)
MY_LFLAGS=
LFLAGS= -licuuc -licudata -licui18n -licuio $(MY_LFLAGS)


default: jazz

jazz: main.o libjazz.a
	$(CC) $(LFLAGS) -o $@ main.o libjazz.a

libjazz.a: lex.o string.o y.tab.o vm.o compile.o type.o
	$(AR) $@ $?
	$(RANLIB) $@


main.o: main.c lex.h string.h vm.h compile.h

type.o: type.c type.h string.h
compile.o: compile.c compile.h parse.h opcode.h type.h \
  vector.h
vm.o: vm.c vm.h opcode.h type.h compile.h vector.h Makefile
lex.o: lex.c lex.h string.h y.tab.h keywords.gp.c Makefile
string.o: string.c string.h lex.h type.h


keywords.gp.c: keywords.gperf
	gperf --output-file=$@ -I -t -C -E $?

y.tab.o: y.tab.c y.tab.h
y.tab.c y.tab.h: parse.y parse.h opcode.h
	$(YACC) parse.y

