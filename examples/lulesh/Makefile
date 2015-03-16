include ../../Makefile.extern
SHELL = /bin/sh
.SUFFIXES: .cc .o

LULESH_EXEC   = lulesh_cd
EXEC_PROFILED = $(LULESH_EXEC)

CD_FLAGS = -D_MPI_VER=1 -D_SINGLE_VER=0 -D_CD=1
#CD_CFLAGS = -I../../src 
#CD_LINKFLAGS = -L../../lib -Wl,-rpath ../../lib -lcds


SERCXX   = g++ -DUSE_MPI=0 
MPICXX   = mpic++ -DUSE_MPI=1 -D_CD=1 -D_DEBUG=1 
CXX      = $(MPICXX) 
CXXFLAGS = -g -std=gnu++0x -openmp -I${SILO_INCDIR} -Wall -Wno-pragmas ${CD_SRC_CFLAGS} ${CD_FLAGS} 
LDFLAGS  = ${CD_LINKFLAGS} ${SIGHT_LINKFLAGS} 

SOURCE_LULESH = \
	./lulesh.cc \
	./lulesh-comm.cc \
	./lulesh-viz.cc \
	./lulesh-util.cc \
	./lulesh-init.cc

LULESH_O      = $(SOURCE_LULESH:.cc=.o)
#TEST_O        = $(SOURCE_TEST:.cc=.o)
#TEST_MPI_O    = $(SOURCE_TEST_MPI:.cc=.o)

#cd_profiler_H := ${CD_ROOT}/src/*.h \
#                 ${CD_ROOT}/lib/libcds.so
#all_H        :=  ${cd_profiler_H}

.cc.o:  
	@echo "\nBuilding $<\n\n"
	$(CXX) -c $(CXXFLAGS) -o $@ $<

all: lulesh_cd

lulesh_cd: $(LULESH_O) 
	@echo "\nLinking"
	$(CXX) -o $@ $^ $(LDFLAGS) -lm

clean:
	rm -rf *.o;
	rm -rf lulesh_cd; 
	rm -rf dbg* 
