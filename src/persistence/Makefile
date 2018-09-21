#!/bin/bash
ENABLE_DEBUG=1

IS_STAMPEDE=1
ifeq (${IS_STAMPEDE}, 1)
#MYCC=g++
MYCC=icpc
MPICC=mpicxx
CFLAGS=-fPIC -Wall -c -std=gnu++0x -I. -fmax-errors=3 #-Weffc++
LFLAGS=-fPIC -Wall -std=gnu++0x -I.
else
MYCC=CC
MPICC=CC
CFLAGS=-fPIC -c -I. -std=gnu++0x
LFLAGS=-fPIC -I.
endif

ifeq (${ENABLE_DEBUG}, 1)
CFLAGS += -g -D_PACKER_DEBUG -O3
LFLAGS += -g -D_PACKER_DEBUG -O3
endif

#CFLAGS += -O3
#all:  test_mpi

OBJS=data_store.o buffer_consumer.o cd_file_handle.o table_store.o packer_prof.o packer_common.o

MPIOBJS=${OBJS} mpi_file_handle.o  cd_file_handle_interface.o

TARGET=libpacker.so

all: ${TARGET}
#all: test_base test_packer test_mpi
${TARGET}: ${MPIOBJS}
	$(CC) -shared -Wl,-soname,libpacker.so $^ -o libpacker.so -lpthread

#test_base: test_base.o ${MPIOBJS}
#	${MPICC} ${LFLAGS} $^ -o $@ -lpthread
#
#test_packer: test_packer.o ${MPIOBJS}
#	${MPICC} ${LFLAGS} $^ -o $@ -lpthread
#
#test_mpi: test_mpi.o ${MPIOBJS} 
#	${MPICC} ${LFLAGS} $^ -o $@ -lpthread 

packer_common.o: packer_common.cc
	${MYCC} ${CFLAGS} $^ -o $@

buffer_consumer.o: buffer_consumer.cc
	${MYCC} ${CFLAGS} $^ -o $@

cd_file_handle.o: cd_file_handle.cc
	${MYCC} ${CFLAGS} $^ -o $@

mpi_file_handle.o: mpi_file_handle.cc
	${MPICC} ${CFLAGS} $^ -o $@

cd_file_handle_interface.o: cd_file_handle_interface.cc
	${MPICC} ${CFLAGS} $^ -o $@

data_store.o: data_store.cc
	${MYCC} ${CFLAGS} $^ -o $@

table_store.o: table_store.cc
	${MYCC} ${CFLAGS} $^ -o $@

packer_prof.o: packer_prof.cc
	${MYCC} ${CFLAGS} $^ -o $@



#test_base.o: test_base.cc
#	${MYCC} ${CFLAGS} $^ -o $@
#
#test_packer.o: test_packer.cc 
#	${MYCC} ${CFLAGS} $^ -o $@
#
#test_mpi.o: test_mpi.cc
#	${MPICC} ${CFLAGS} $^ -o $@


# cd_file_handle_interface is only for GetFileHandle, need to fix it soon.
# data_store.o is only for active_buffer definition
# buffer_consumer is only for mutex definition
#test_mpifile: mpi_file_handle.o cd_file_handle.o buffer_consumer.o data_store.o cd_file_handle_interface.o
#	${MPICC} ${CFLAGS} -I./utils test_mpi_file_handle.cc -o test_mpi_file_handle.o
#	${MPICC} ${LFLAGS} test_mpi_file_handle.o $^ -o test_mpi_file_handle -lpthread
clean:
	rm *.o; rm test_base; rm test_packer; rm test_mpi; rm test_mpi_file_handle

clean_result:
	rm posix_filepath.*; rm test.ref.*; mpi_filepath.*;
