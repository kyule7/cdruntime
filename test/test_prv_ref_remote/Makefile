include ../../Makefile.extern

CD_APP_FLAGS=-D_ERROR_INJECTION_ENABLED=1

all:
	mpic++  -std=gnu++0x -o test_prv_ref_remote ${CD_CFLAGS} ${CD_APP_FLAGS} ./test_prv_ref_remote.cc ${CD_LINKFLAGS}

clean:
	rm -rf  test_prv_ref_remote; rm -rf HDD; rm -rf output/output_*

clean_cdruntime:
	cd ../../src; make clean;

refresh:
	cd ../../src; make clean; make DEBUG_VAR=1 MPI_VER_VAR=1 SINGLE_VER_VAR=0 LOGGING_ENABLED=1; ERROR_INJECTION_ENABLED=1
