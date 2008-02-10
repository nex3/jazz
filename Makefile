MAKE= make

default: all

all:
	cd src && $(MAKE)
	ln -fs src/jazz jazz

clean: clean-except-gcov
	rm -rf src/*.gc* src/core/*.gc* src/y.tab.*

clean-except-gcov:
	rm -rf jazz src/jazz src/*.a src/core/*.a src/keywords.gp.c  src/*.o src/core/*.o coverage/

test: all never_up_to_date
	bash test/test.sh

gcov: clean
	cd src && $(MAKE) MY_CFLAGS="-ftest-coverage -fprofile-arcs" MY_LFLAGS="-ftest-coverage -fprofile-arcs"
	$(MAKE) test
	cd src && gcov *.c
	cd src/core && gcov *.c
	$(MAKE) clean-except-gcov

coverage: gcov
	mkdir -p coverage/
	cd src && lcov -c -d . -b . -o ../coverage/lcov.info
	lcov -r coverage/lcov.info y.tab.c -o coverage/lcov.info
	genhtml -o coverage/ coverage/lcov.info

never_up_to_date:
