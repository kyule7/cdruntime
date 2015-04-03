include ../../Makefile.extern
SHELL = /bin/sh

FLAGS = -D_MPI_VER=1 \
        -D_SINGLE_VER=0 \
        -D_DEBUG=1


all:
	mpic++ -g -std=gnu++0x -o test_simple_hierarchy ${CD_CFLAGS} ${FLAGS} ./test_simple_hierarchy.cc ${CD_LINKFLAGS} 

refresh:
	cd ../../src; make clean; make DEBUG_VAR=1 MPI_VER_VAR=1 SINGLE_VER_VAR=0 LOGGING_ENABLED=1;
	mpic++ -g -std=gnu++0x -o test_simple_hierarchy ${CD_CFLAGS} ${FLAGS} ./test_simple_hierarchy.cc ${CD_LINKFLAGS} 


clean:
	rm -rf *.o; rm output/output_*; rm test_simple_hierarchy

