#default build suggestion of MPI + OPENMP with gcc on Livermore machines you might have to change the compiler name

SHELL = /bin/sh
.SUFFIXES: .cc .o

CD_ENABLED = 1
ifeq (${CD_ENABLED}, 1)
CD_FLAGS = -D_MPI_VER=1 -D_SINGLE_VER=0 -D_CD=${CD_ENABLED} -D_DEBUG=0 
CD_CFLAGS = -I${CD_PATH}/include -I${CD_PATH}/src -I${CD_PATH}/src/persistence
CD_LINKFLAGS = -L${CD_PATH}/lib -lcd -Wl,-rpath=${CD_PATH}/lib -L${CD_PATH}/src/persistence -lpacker -Wl,-rpath=${CD_PATH}/src/persistence -ldl
else
CD_FLAGS := 
CD_CFLAGS :=
CD_LINKFLAGS :=
endif

#CD_CFLAGS += -DOPTIMIZE_PRV
LULESH_EXEC = lulesh_cd

MPI_INC = /opt/local/include/openmpi
MPI_LIB = /opt/local/lib

SERCXX = g++ -DUSE_MPI=0
MPICXX = mpicxx -DUSE_MPI=1
CXX = $(MPICXX)

SOURCES2.0 = \
	lulesh.cc \
	lulesh-comm.cc \
	lulesh-viz.cc \
	lulesh-util.cc \
	lulesh-init.cc
OBJECTS2.0 = $(SOURCES2.0:.cc=.o)

#Default build suggestions with OpenMP for g++
CXXFLAGS = -g -std=gnu++0x  -Wall -Wno-pragmas -fmax-errors=1 ${CD_CFLAGS} ${CD_FLAGS} 
LDFLAGS = -g -O3  ${CD_LINKFLAGS}
CXXFLAGS += -fopenmp
LDFLAGS += -fopenmp
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

$(LULESH_EXEC): $(OBJECTS2.0)
	@echo "Linking"
	$(CXX) $(OBJECTS2.0) $(LDFLAGS) -lm -o $@

clean:
	rm -rf *.o;
	rm -rf lulesh_cd; 
	rm -rf cddbg* 
