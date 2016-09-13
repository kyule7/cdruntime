#include ../../Makefile.extern
SHELL = /bin/sh
.SUFFIXES: .cc .o
CD_PATH=/home1/03341/kyushick/Work/CDs/overheadFix
INTERVAL_VAL=0
LULESH_EXEC   = lulesh_cd
EXEC_PROFILED = $(LULESH_EXEC)

CD_FLAGS = -D_MPI_VER=1 -D_SINGLE_VER=0 -D_CD=1 -D_DEBUG=0
CD_CFLAGS = -I${CD_PATH}/include 
CD_LINKFLAGS = -L${CD_PATH}/lib -lcd -Wl,-rpath=${CD_PATH}/lib


SERCXX   = g++ -DUSE_MPI=0 
MPICXX   = mpic++ -DUSE_MPI=1 
CXX      = $(MPICXX) 
CXXFLAGS = -g -std=gnu++0x  -I${SILO_INCDIR} -Wall -Wno-pragmas -fmax-errors=1 ${CD_CFLAGS} ${CD_FLAGS} 
LDFLAGS  = ${CD_LINKFLAGS}



#LDFLAGS  +=  ${VIZ_LINKFLAGS} -lgomp
#CD_CFLAGS += ${SIGHT_CFLAGS} 

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
#                 ${CD_ROOT}/lib/libcd.so
#all_H        :=  ${cd_profiler_H}

.cc.o:  
	@echo "\nBuilding $<\n\n"
	$(CXX) -c $(CXXFLAGS) -o $@ $<

all: lulesh_cd

lulesh_cd: $(LULESH_O) 
	@echo "\nLinking"
	$(CXX) -DINTERVAL=${INTERVAL_VAL} -o $@ $^ $(LDFLAGS) -lm

clean:
	rm -rf *.o;
	rm -rf lulesh_cd; 
	rm -rf cddbg* 
