CC=g++
CFLAGS=-std=gnu++0x -g -I..
LFLAGS=-L.. -lwrapLibc -Wl,-rpath=..

EXE=test_wrapper.exe test_genid.exe test_ul.exe
all: ${EXE}

test_wrapper.exe: test_wrapper.cc
	${CC} ${CFLAGS} test_wrapper.cc -o test_wrapper.exe ${LFLAGS}

test_genid.exe: test_genid.cc
	${CC} ${CFLAGS} test_genid.cc -o test_genid.exe ${LFLAGS}

test_ul.exe: test_ul.cc
	${CC} ${CFLAGS} test_ul.cc -o test_ul.exe ${LFLAGS}

clean:
	rm *.exe
