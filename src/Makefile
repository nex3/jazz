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

libjazz.a: lex.o string.o y.tab.o vm.o compile.o value.o state.o frame.o gc.o
	$(AR) $@ $?
	$(RANLIB) $@


main.o: main.c state.h string.h parse.h compile.h vm.h

value.o: value.c value.h string.h
compile.o: compile.c compile.h string.h
vm.o: vm.c vm.h frame.h state.h string.h gc.h Makefile
lex.o: lex.c lex.h state.h value.h string.h y.tab.h keywords.gp.c Makefile
string.o: string.c string.h lex.h
state.o: state.c state.h
frame.o: frame.c frame.h
gc.o: gc.c gc.h state.h string.h

%.h:
	touch $@

jazz.h:
gc.h: jazz.h value.h
vector.h: jazz.h
value.h: jazz.h
parse.h: jazz.h value.h
lex.h: jazz.h y.tab.h
string.h: jazz.h value.h gc.h
frame.h: jazz.h compile.h value.h
compile.h: jazz.h parse.h opcode.h vector.h	
vm.h: jazz.h value.h compile.h
state.h: jazz.h value.h frame.h gc.h

keywords.gp.c: keywords.gperf
	gperf --output-file=$@ -I -t -C -E $?

y.tab.o: y.tab.c y.tab.h
y.tab.c y.tab.h: parse.y parse.h state.h string.h
	$(YACC) parse.y

