#!/bin/bash
CC=g++
MPICC=mpic++
CFLAGS=-Wall -g -c -std=gnu++0x -I. -fmax-errors=3
LFLAGS=-Wall -g -std=gnu++0x -I.

all: test_base test_packer test_mpi
#all:  test_mpi

OBJS=data_store.o buffer_consumer.o cd_file_handle.o table_store.o

MPIOBJS=${OBJS} mpi_file_handle.o  cd_file_handle_interface.o
test_base: test_base.o ${MPIOBJS}
	${MPICC} ${LFLAGS} $^ -o $@ -lpthread

test_packer: test_packer.o ${MPIOBJS}
	${MPICC} ${LFLAGS} $^ -o $@ -lpthread

test_mpi: test_mpi.o ${MPIOBJS} 
	${MPICC} ${LFLAGS} $^ -o $@ -lpthread 

buffer_consumer.o: buffer_consumer.cc
	${CC} ${CFLAGS} $^ -o $@

cd_file_handle.o: cd_file_handle.cc
	${CC} ${CFLAGS} $^ -o $@

mpi_file_handle.o: mpi_file_handle.cc
	${MPICC} ${CFLAGS} $^ -o $@

cd_file_handle_interface.o: cd_file_handle_interface.cc
	${MPICC} ${CFLAGS} $^ -o $@

data_store.o: data_store.cc
	${CC} ${CFLAGS} $^ -o $@

table_store.o: table_store.cc
	${CC} ${CFLAGS} $^ -o $@

test_base.o: test_base.cc
	${CC} ${CFLAGS} $^ -o $@

test_packer.o: test_packer.cc 
	${CC} ${CFLAGS} $^ -o $@

test_mpi.o: test_mpi.cc
	${MPICC} ${CFLAGS} $^ -o $@


# cd_file_handle_interface is only for GetFileHandle, need to fix it soon.
# data_store.o is only for active_buffer definition
# buffer_consumer is only for mutex definition
test_mpifile: mpi_file_handle.o cd_file_handle.o buffer_consumer.o data_store.o cd_file_handle_interface.o
	${MPICC} ${CFLAGS} -I./utils test_mpi_file_handle.cc -o test_mpi_file_handle.o
	${MPICC} ${LFLAGS} test_mpi_file_handle.o $^ -o test_mpi_file_handle -lpthread
clean:
	rm *.o; rm test_base; rm test_packer; rm test_mpi; rm test_mpi_file_handle

clean_result:
	rm posix_filepath.*; rm test.ref.*; mpi_filepath.*;
