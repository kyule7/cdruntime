#CC=g++
CC=mpicxx
MPICC=mpicxx
CDROOT=/home1/03341/kyushick/Work/CDs/libc
CFLAGS=-std=gnu++0x -g -O0 -I.. -I${CDROOT}/src/persistence
LFLAGS=-L.. -lwrapLibc -Wl,-rpath=.. -L${CDROOT}/src/persistence -lpacker -Wl,-rpath=${CDROOT}/src/persistence -lc
NEW_LFLAGS=-L${CDROOT}/src/persistence -lpacker -Wl,-rpath=${CDROOT}/src/persistence

EXE=test_wrapper.exe test_genid.exe test_ul.exe test_push.exe test_mpi.exe
all: ${EXE}

test_wrapper.exe: test_wrapper.cc
	${CC} ${CFLAGS} test_wrapper.cc -o test_wrapper.exe ${LFLAGS}

test_genid.exe: test_genid.cc
	${CC} ${CFLAGS} test_genid.cc -o test_genid.exe ${LFLAGS}

test_ul.exe: test_ul.cc
	${CC} ${CFLAGS} test_ul.cc -o test_ul.exe ${LFLAGS}

test_push.exe: test_push.cc
	${CC} ${CFLAGS} test_push.cc -o test_push.exe ${LFLAGS}

test_mpi.exe: test_mpi.cc
	${MPICC} ${CFLAGS} test_mpi.cc -o test_mpi.exe ${LFLAGS}

clean:
	rm *.exe
