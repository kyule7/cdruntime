include ../../Makefile.extern
SHELL = /bin/sh

CD_APP_FLAGS=-D_ERROR_INJECTION_ENABLED=1

all:
	mpic++  -std=gnu++0x -o test_user_defined_hierarchy ${CD_CFLAGS} ${CD_APP_FLAGS} ./test_user_defined_hierarchy.cc ${CD_LINKFLAGS}

clean:
	rm -rf  test_user_defined_hierarchy; rm -rf ./HDD 

clean_cdruntime:
	cd ../../src; make clean;

refresh:
	cd ../../src; make clean; make DEBUG_VAR=1 MPI_VER_VAR=1 SINGLE_VER_VAR=0 LOGGING_ENABLED=0; ERROR_INJECTION_ENABLED=1





