#default build suggestion of MPI + OPENMP with gcc on Livermore machines you might have to change the compiler name

SHELL = /bin/sh
.SUFFIXES: .cc .o

CDPATH := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))/../..
CD_ENABLED = 1
#LULESH_EXEC = lulesh_nocd
#LULESH_EXEC = lulesh_bare
#LULESH_EXEC = lulesh_noprv
#LULESH_EXEC = lulesh_opt
#LULESH_EXEC = lulesh_fgcd
LULESH_EXEC = lulesh_scr
#LULESH_EXEC = lulesh_lv2
#LULESH_EXEC = lulesh_cdrt
#LULESH_EXEC = lulesh_main
#LULESH_EXEC = lulesh_incr
#LULESH_EXEC = lulesh_full
ifeq (${CD_ENABLED}, 1)
CD_CDRT = 1
CD_INCR_CKPT = 0
CD_FULL_CKPT = 0

CD_OPS = -D_CD_CDRT=${CD_CDRT} \
				 -D_CD_INCR_CKPT=${CD_INCR_CKPT} \
				 -D_CD_FULL_CKPT=${CD_FULL_CKPT} \
				 -D_CD_ROOT=1 \
				 -D_CD_CHILD=1 \
				 -D_LEAF_LV=1 \
  			 -D_POS_VEL_ACC=0 \
  			 -D_LAGRANGE_ELEM=0 \
  			 -D_CALC_FOR_ELEM=0 \
  			 -D_MATERIAL_PROP=0 \
				 -D_SCR=1 \
				 -D_CD_DUMMY=0 \
				 -D_FGCD=0
CD_FLAGS = -D_MPI_VER=1 -D_SINGLE_VER=0 -D_CD=${CD_ENABLED} -D_DEBUG=0 ${CD_OPS} 
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

#ifeq (${CD_CHILD}, 1)
#LULESH_EXEC = lulesh_cdrt
#else
#LULESH_EXEC = lulesh_noprv
#endif
#
#ifeq ($(CD_INCR_CKPT), 1)
#LULESH_EXEC = lulesh_incr
#else
#LULESH_EXEC = lulesh_1
#endif
#
#ifeq (${CD_FULL_CKPT}, 1)
#LULESH_EXEC = lulesh_full
#endif

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
	lulesh-comm.cc \
	lulesh-viz.cc \
	lulesh-util.cc \
	lulesh-init.cc
OBJECTS2.0 = $(SOURCES2.0:.cc=.o) lulesh.o

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

all: $(LULESH_EXEC)

.cc.o: lulesh.h $(OBJECTS2.0)
	@echo "Building $<"
	$(CXX) -c $(CXXFLAGS) -o $@  $<

lulesh.o: lulesh.cc lulesh.h cd_def.h Makefile 
	@echo "Building $< OBJS $(OBJECTS2.0)"
	$(CXX) -c $(CXXFLAGS) -o $@  $<

$(LULESH_EXEC): $(OBJECTS2.0)
	@echo "Linking for $(LULESH_EXEC)"
	$(CXX) $(OBJECTS2.0) $(LDFLAGS) -lm -o $@

clean:
	/bin/rm -f *.o *~ $(OBJECTS) $(LULESH_EXEC)
	/bin/rm -rf *.dSYM

tar: clean
	cd .. ; tar cvf lulesh-2.0.tar LULESH-2.0 ; mv lulesh-2.0.tar LULESH-2.0

