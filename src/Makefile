CC= gcc
YACC= bison -y -d
AR= ar rcu
RANLIB= ranlib

MY_CFLAGS=
CFLAGS= -g -ansi -Wall -pedantic -iquote '.' -DJZ_DEBUG_LEX=0 -DJZ_DEBUG_BYTECODE=0 $(MY_CFLAGS)
MY_LFLAGS=
LFLAGS= -licuuc -licudata -licui18n -licuio $(MY_LFLAGS)


default: jazz

jazz: main.o libjazz.a core/core.a
	$(CC) $(LFLAGS) -o $@ main.o libjazz.a core/core.a

libjazz.a: lex.o string.o y.tab.o vm.o compile.o value.o state.o frame.o gc.o \
  object.o prototype.o function.o
	$(AR) $@ $?
	$(RANLIB) $@

core/core.a: core/core.o core/global.o
	$(AR) $@ $?
	$(RANLIB) $@


main.o: main.c state.h string.h parse.h compile.h vm.h core/core.h

value.o: value.c value.h string.h object.h
compile.o: compile.c compile.h string.h
vm.o: vm.c vm.h frame.h state.h string.h gc.h object.h Makefile
lex.o: lex.c lex.h state.h value.h string.h y.tab.h keywords.gp.c Makefile
string.o: string.c string.h lex.h gc.h state.h
state.o: state.c state.h lex.h object.h function.h
frame.o: frame.c frame.h
gc.o: gc.c gc.h state.h string.h object.h
object.o: object.c object.h string.h gc.h prototype.h
prototype.o: prototype.c prototype.h state.h
function.o: function.c function.h object.h prototype.h

core/core.o: core/core.c core/core.h core/global.h
core/global.o: core/global.c core/global.h state.h function.h object.h

%.h:
	touch $@

jazz.h:
gc.h: jazz.h value.h
vector.h: jazz.h
value.h: jazz.h
parse.h: jazz.h value.h
lex.h: jazz.h value.h y.tab.h
string.h: jazz.h value.h gc.h
frame.h: jazz.h compile.h value.h
compile.h: jazz.h parse.h opcode.h vector.h	
vm.h: jazz.h value.h compile.h
state.h: jazz.h value.h frame.h gc.h
object.h: jazz.h gc.h value.h string.h function.h
prototype.h: jazz.h object.h
function.h: jazz.h value.h

core/core.h: jazz.h
core/global.h: jazz.h

keywords.gp.c: keywords.gperf
	gperf --output-file=$@ -I -t -C -E $?

y.tab.o: y.tab.c y.tab.h
y.tab.c y.tab.h: parse.y parse.h state.h string.h
	$(YACC) parse.y

