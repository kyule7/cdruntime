#!/bin/bash
CC=g++
MPICC=mpic++
CFLAGS=-Wall -g -c -std=gnu++0x -I. 
LFLAGS=-Wall -g -std=gnu++0x -I.

#all: test_base test_packer test_mpi
all:  test_mpi

OBJS=data_store.o buffer_consumer.o cd_file_handle.o  # cd_file_handle_interface.o
MPIOBJS=${OBJS} mpi_file_handle.o # cd_file_handle_interface.o
test_base: test_base.o ${OBJS}
	${CC} ${LFLAGS} $^ -o $@ -lpthread

test_packer: test_packer.o ${OBJS}
	${CC} ${LFLAGS} $^ -o $@ -lpthread

test_mpi: test_mpi.o ${MPIOBJS} 
	${MPICC} ${LFLAGS} $^ -o $@ -lpthread 

buffer_consumer.o: buffer_consumer.cc
	${CC} ${CFLAGS} $^ -o $@

cd_file_handle.o: cd_file_handle.cc
	${CC} ${CFLAGS} $^ -o $@

mpi_file_handle.o: mpi_file_handle.cc
	${MPICC} ${CFLAGS} $^ -o $@

#cd_file_handle_interface.o: cd_file_handle_interface.cc
#	${MPICC} ${CFLAGS} $^ -o $@

data_store.o: data_store.cc
	${CC} ${CFLAGS} $^ -o $@

test_base.o: test_base.cc
	${CC} ${CFLAGS} $^ -o $@

test_packer.o: test_packer.cc 
	${CC} ${CFLAGS} $^ -o $@

test_mpi.o: test_mpi.cc
	${MPICC} ${CFLAGS} $^ -o $@

test_mpifile: mpi_file_handle.o
	${MPICC} ${CFLAGS} -I./utils test_mpi_file_handle.cc -o test_mpi_file_handle.o
	${MPICC} ${LFLAGS} test_mpi_file_handle.o mpi_file_handle.o -o test_mpi_file_handle
clean:
	rm *.o; rm test_base; rm test_packer; rm test_mpi; rm test_mpi_file_handle;

clean_result:
	rm posix_filepath.*; rm test.ref.*; mpi_filepath.*;
