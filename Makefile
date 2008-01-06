CC= gcc
YACC= bison -y -d
AR= ar rcu
RANLIB= ranlib
MAKE= make

MY_CFLAGS=
CFLAGS= -g -ansi -Wall -pedantic -DJZ_DEBUG_LEX=0 -DJZ_DEBUG_BYTECODE=0 $(MY_CFLAGS)
MY_LFLAGS=
LFLAGS= -licuio $(MY_LFLAGS)


default: jazz

clean: clean-except-gcov
	rm -rf src/*.gc*

clean-except-gcov:
	rm -rf jazz libjazz.* src/keywords.gp.c src/y.tab.* src/*.o coverage/

test: jazz never_up_to_date
	bash test/test.sh

coverage: clean
	$(MAKE) MY_CFLAGS="-ftest-coverage -fprofile-arcs" MY_LFLAGS="-ftest-coverage -fprofile-arcs"
	$(MAKE) test
	gcov -o src/ `find src/ -regex '[^.]*\.c'`
	cd src && gcov y.tab.c
	mv *.gcov src/
	$(MAKE) clean-except-gcov

lcov: coverage
	mkdir -p coverage/
	lcov -c -d src/ -b . -o coverage/lcov.info
	genhtml -o coverage/ coverage/lcov.info


jazz: src/main.o libjazz.a
	$(CC) $(LFLAGS) -o $@ src/main.o libjazz.a

libjazz.a: src/lex.o src/string.o src/y.tab.o src/vm.o src/compile.o src/type.o
	$(AR) $@ $?
	$(RANLIB) $@

never_up_to_date:

src/main.o: src/main.c src/lex.h src/string.h src/vm.h src/compile.h

src/type.o: src/type.c src/type.h
src/compile.o: src/compile.c src/compile.h src/parse.h src/opcode.h src/type.h \
  src/vector.h
src/vm.o: src/vm.c src/vm.h src/opcode.h src/type.h src/compile.h src/vector.h \
  Makefile
src/lex.o: src/lex.c src/lex.h src/string.h src/y.tab.h src/keywords.gp.c \
  Makefile
src/string.o: src/string.c src/string.h


src/keywords.gp.c: src/keywords.gperf
	gperf --output-file=$@ -I -t -C -E $?

src/y.tab.o: src/y.tab.c src/y.tab.h
src/y.tab.c src/y.tab.h: src/parse.y src/parse.h src/opcode.h
	cd src && $(YACC) parse.y
