#default build suggestion of MPI + OPENMP with gcc on Livermore machines you might have to change the compiler name

SHELL = /bin/sh
.SUFFIXES: .cc .o

CDPATH := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))/../..
CD_ENABLED = 1
ifeq (${CD_ENABLED}, 1)
CD_FLAGS = -D_MPI_VER=1 -D_SINGLE_VER=0 -D_CD=${CD_ENABLED} -D_DEBUG=0 -D_LEAF_LV=0
CD_CFLAGS = -std=gnu++0x -fmax-errors=1 -fPIC -I${CDPATH}/include -I${CDPATH}/src -I${CDPATH}/src/persistence -I${CDPATH}/plugin/wrapper
#WRAPPER_LFLAGS = -lwrapLibc -L${CDPATH}/plugin/wrapper -Wl,-rpath=${CDPATH}/plugin/wrapper -lc 
WRAPPER_LFLAGS = -lwrapLibc -L${CDPATH}/plugin/wrapper -Wl,-rpath=${CDPATH}/plugin/wrapper -lc 
CD_LFLAGS = -L${CDPATH}/lib -lcd -Wl,-rpath=${CDPATH}/lib -L${CDPATH}/src/persistence -lpacker -Wl,-rpath=${CDPATH}/src/persistence -ldl
CD_LINKFLAGS = ${WRAPPER_LFLAGS} ${CD_LFLAGS}
else
CD_FLAGS := 
CD_CFLAGS :=
CD_LINKFLAGS :=
endif

LULESH_EXEC = lulesh_cd

MPI_INC = /opt/local/include/openmpi
MPI_LIB = /opt/local/lib


IS_STAMPEDE=1
ifeq (${IS_STAMPEDE}, 1)
SERCXX = g++ -DUSE_MPI=0
MPICXX = mpicxx -DUSE_MPI=1 -fmax-errors=3
else
SERCXX = cc -DUSE_MPI=0
MPICXX = CC -DUSE_MPI=1
CD_LINKFLAGS += -dynamic
endif
CXX = $(MPICXX)

SOURCES2.0 = \
	lulesh.cc \
	lulesh-comm.cc \
	lulesh-viz.cc \
	lulesh-util.cc \
	lulesh-init.cc
OBJECTS2.0 = $(SOURCES2.0:.cc=.o)

#Default build suggestions with OpenMP for g++
CXXFLAGS = -g -O3 -I. -Wall
LDFLAGS = -g -O3 
CXXFLAGS += ${CD_CFLAGS} ${CD_FLAGS} 
LDFLAGS  += ${CD_LINKFLAGS}
CXXFLAGS += -fopenmp
LDFLAGS  += -fopenmp
#Below are reasonable default flags for a serial build
#CXXFLAGS = -g -O3 -I. -Wall
#LDFLAGS = -g -O3 

#common places you might find silo on the Livermore machines.
#SILO_INCDIR = /opt/local/include
#SILO_LIBDIR = /opt/local/lib
#SILO_INCDIR = ./silo/4.9/1.8.10.1/include
#SILO_LIBDIR = ./silo/4.9/1.8.10.1/lib

#If you do not have silo and visit you can get them at:
#silo:  https://wci.llnl.gov/codes/silo/downloads.html
#visit: https://wci.llnl.gov/codes/visit/download.html

#below is and example of how to make with silo, hdf5 to get vizulization by default all this is turned off.  All paths are Livermore specific.
#CXXFLAGS = -g -DVIZ_MESH -I${SILO_INCDIR} -Wall -Wno-pragmas
#LDFLAGS = -g -L${SILO_LIBDIR} -Wl,-rpath -Wl,${SILO_LIBDIR} -lsiloh5 -lhdf5

.cc.o: lulesh.h
	@echo "Building $<"
	$(CXX) -c $(CXXFLAGS) -o $@  $<

all: $(LULESH_EXEC)

lulesh_cd: $(OBJECTS2.0)
	@echo "Linking"
	$(CXX) $(OBJECTS2.0) $(LDFLAGS) -lm -o $@

clean:
	/bin/rm -f *.o *~ $(OBJECTS) $(LULESH_EXEC)
	/bin/rm -rf *.dSYM

tar: clean
	cd .. ; tar cvf lulesh-2.0.tar LULESH-2.0 ; mv lulesh-2.0.tar LULESH-2.0

