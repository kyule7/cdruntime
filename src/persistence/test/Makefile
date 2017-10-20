#!/bin/bash
CC=mpic++
MPICC=mpic++
CFLAGS=-fPIC -Wall -g -c -std=gnu++0x -I. -I.. -fmax-errors=3
LFLAGS=-fPIC -Wall -g -std=gnu++0x
LFLAGS_EXTRA=-L.. -Wl,-rpath=.. -lpacker -lpthread

TARGET=test_base test_packer test_mpi

all: ${TARGET}

test_base: test_base.o ${MPIOBJS}
	${MPICC} ${LFLAGS} $^ -o $@ ${LFLAGS_EXTRA} 

test_packer: test_packer.o ${MPIOBJS}
	${MPICC} ${LFLAGS} $^ -o $@ ${LFLAGS_EXTRA}

test_mpi: test_mpi.o ${MPIOBJS} 
	${MPICC} ${LFLAGS} $^ -o $@ ${LFLAGS_EXTRA}

test_base.o: test_base.cc
	${CC} ${CFLAGS} $^ -o $@

test_packer.o: test_packer.cc
	${CC} ${CFLAGS} $^ -o $@

test_mpi.o: test_mpi.cc
	${MPICC} ${CFLAGS} $^ -o $@

clean:
	rm ${TARGET}; rm test.ref.*; rm *.o; rm prv_*;
