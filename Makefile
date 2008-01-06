MAKE= make

default: all

all:
	cd src && $(MAKE)
	mv src/jazz src/libjazz.a .

clean: clean-except-gcov
	rm -rf src/*.gc* src/y.tab.*

clean-except-gcov:
	rm -rf jazz libjazz.* src/keywords.gp.c  src/*.o coverage/

test: all never_up_to_date
	bash test/test.sh

gcov: clean
	cd src && $(MAKE) MY_CFLAGS="-ftest-coverage -fprofile-arcs" MY_LFLAGS="-ftest-coverage -fprofile-arcs"
	$(MAKE) test
	cd src && gcov *.c
	$(MAKE) clean-except-gcov

coverage: gcov
	mkdir -p coverage/
	cd src && lcov -c -d . -o ../coverage/lcov.info
	genhtml -o coverage/ coverage/lcov.info

never_up_to_date:
