include ../../Makefile.extern
SHELL = /bin/sh

CD_APP_FLAGS=-D_ERROR_INJECTION_ENABLED=0 -D_PROFILER=1
#PROFILER_ENABLED=1
#
#ifeq ($(PROFILER_ENABLED), 1)
#PROFILER_FLAGS = -D_PROFILER=1\
#      -D_ENABLE_MODULE=1 \
#      -D_ENABLE_HIERGRAPH=0 \
#      -D_ENABLE_SCOPE=0 \
#      -D_ENABLE_GRAPH=0 \
#      -D_ENABLE_ATTR=0 \
#      -D_ENABLE_COMP=1 \
#      -D_ENABLE_CDNODE=0
#endif

CD_CFLAGS += ${SIGHT_CFLAGS}  
CD_LINKFLAGS += ${VIZ_LINKFLAGS}
all:
	mpic++  -std=gnu++0x -o test_simple_hierarchy ${CD_CFLAGS} ${CD_APP_FLAGS} ./test_simple_hierarchy.cc ${CD_LINKFLAGS}

clean:
	rm -rf  test_simple_hierarchy; rm -rf ./HDD; rm debug_logs/dbg_log_rank*; rm -rf debug_logs/output_*

clean_cdruntime:
	cd ../../src; make clean;

refresh:
	cd ../../src; make clean; make DEBUG_VAR=1 MPI_VER_VAR=1 SINGLE_VER_VAR=0 LOGGING_ENABLED=0 ERROR_INJECTION_ENABLED=0 PROFILER_ENABLED=1;








