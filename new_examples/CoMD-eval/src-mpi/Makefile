# Makefile for src-parallel

SHELL = /bin/bash

### CoMD can be built in either double or single precision and with or
### without MPI.  Select desired precision and MPI here.

CD_ENABLED = 1
# double precision (ON/OFF)
DOUBLE_PRECISION = ON
# MPI for parallel (ON/OFF)
DO_MPI = ON

### Set your desired C compiler and any necessary flags.  Note that CoMD
### uses some c99 features.  You can also set flags for optimization and
### specify paths to include files that the compiler can't find on its
### own.  If you need any -L or -l switches to get C standard libraries
### (such as -lm for the math library) put them in C_LIB.
CC = mpicc

CFLAGS = -std=gnu11
#CFLAGS = -std=c11
#CD_ROOT=/home1/03341/kyushick/Work/CDs/pfsFix
#CD_ROOT=../../../
CD_ROOT=/work/03710/ykwon/stampede2/SPRING18/cdresilience
ifeq (${CD_ENABLED}, 1)
CD_FLAGS := 
# ROOTCD : CoMD.c
# CD1 : timestep.c 
# CD2 : ljForce.c

CD_CFLAGS := -I$(CD_ROOT)/include -D_ROOTCD=1 -D_CD1=1 -D_CD1_OUTPUT -D_CD2=1 -D_CD2_OUTPUT -D_CD3=1 -D_CD4=1 -DDO_PRV -D_CD3_NO_SPLIT
#CD_CFLAGS := -I$(CD_ROOT)/include -D_ROOTCD=1 -D_CD1=1 -D_CD1_OUTPUT -D_CD2=1 -D_CD2_OUTPUT -D_CD3=1 -D_CD4=1 -DDO_PRV
#CD_CFLAGS := -I$(CD_ROOT)/include -D_ROOTCD=1 -D_CD1=1 -D_CD2=1 -D_CD3=1 -D_CD4=1 -DDO_PRV

#CD_CFLAGS := -I$(CD_ROOT)/include -D_ROOTCD=1 -D_CD1=1 -D_CD1_OUTPUT -D_CD2=1 -D_CD3=1 -D_CD4=1 -DDO_PRV
#CD_CFLAGS := -I$(CD_ROOT)/include -D_ROOTCD=1 -D_CD1=1 -D_CD1_OUTPUT -D_CD2=1 -D_CD3=1 -D_CD4=1

#CD_CFLAGS := -I$(CD_ROOT)/include -D_ROOTCD=1 -D_CD1=1 -D_CD2=1 -D_CD3=1 -D_CD4=1 -DDO_PRV -DDO_OUTPUT
#CD_CFLAGS := -I$(CD_ROOT)/include -D_ROOTCD=1 -D_CD1=1 -D_CD2=1 -D_CD3=1 -D_CD4=1 -DDO_PRV
#CD_CFLAGS := -I$(CD_ROOT)/include -D_ROOTCD=1 -D_CD1=1 -D_CD2=1 -D_CD3=1 -D_CD4=1

#FIXME: Level 3 should be enabled with Level 4
#CD_CFLAGS := -I$(CD_ROOT)/include -D_ROOTCD=1 -D_CD1=1 -D_CD1_OUTPUT -D_CD2=1 -D_CD3=1 -DDO_PRV
#CD_CFLAGS := -I$(CD_ROOT)/include -D_ROOTCD=1 -D_CD1=1 -D_CD2=1 -D_CD3=1 -DDO_PRV
#CD_CFLAGS := -I$(CD_ROOT)/include -D_ROOTCD=1 -D_CD1=1 -D_CD2=1 -D_CD3=1

#CD_CFLAGS := -I$(CD_ROOT)/include -D_ROOTCD=1 -D_CD1=1 -D_CD1_OUTPUT -D_CD2=1 -DDO_PRV
#CD_CFLAGS := -I$(CD_ROOT)/include -D_ROOTCD=1 -D_CD1=1 -D_CD2=1 -DDO_PRV
#CD_CFLAGS := -I$(CD_ROOT)/include -D_ROOTCD=1 -D_CD1=1 -D_CD2=1 

#CD_CFLAGS := -I$(CD_ROOT)/include -D_ROOTCD=1 -D_CD1=1 -DDO_PRV -D_CD1_OUTPUT
#CD_CFLAGS := -I$(CD_ROOT)/include -D_ROOTCD=1 -D_CD1=1 -DDO_PRV
#CD_CFLAGS := -I$(CD_ROOT)/include -D_ROOTCD=1 -D_CD1=1 

#CD_CFLAGS := -I$(CD_ROOT)/include -D_ROOTCD=1 -DDO_PRV 
#CD_CFLAGS := -I$(CD_ROOT)/include -D_ROOTCD=1 

#CD_CFLAGS := -I$(CD_ROOT)/include 

CD_LINKFLAGS := -L$(CD_ROOT)/lib -lcd -Wl,-rpath=$(CD_ROOT)/lib -L$(CD_ROOT)/src/persistence -lpacker -Wl,-rpath=$(CD_ROOT)/src/persistence
#CD_LINKFLAGS := -L$(CD_ROOT)/lib -lcd -Wl,-rpath=$(CD_ROOT)/lib -L$(CD_ROOT)/src/persistence -lpacker -Wl,-rpath=$(CD_ROOT)/src/persistence -dynamic
#CD_LINKFLAGS := -L$(CD_ROOT)/lib -lwrapLibc -lc -lcd -Wl,-rpath=$(CD_ROOT)/lib -L$(CD_ROOT)/src/persistence -lpacker -Wl,-rpath=$(CD_ROOT)/src/persistence
else
CD_FLAGS := 
CD_CFLAGS :=
CD_LINKFLAGS :=
endif

CFLAGS += ${CD_CFLAGS} ${CD_FLAGS} #-gdwarf-2
#OPTFLAGS = -g -O5
OPTFLAGS = -g -O3
INCLUDES = 
C_LIB = -lm ${CD_LINKFLAGS}


### If you need to specify include paths, library paths, or link flags
### for MPI, put them here.  Put both -L and -l switches into MPI_LIB.
MPI_LIB =
MPI_INCLUDE =

### A place to specify any other include or library switches your
### platform requires.
OTHER_LIB =
OTHER_INCLUDE =




#########################################
### Below here, it is pitch black.  
### You are likely to be eaten by a grue.
##########################################

# clear all suffixes
.SUFFIXES:
# list only those that we use 
.SUFFIXES: .c .o

.PHONY: DEFAULT clean distclean depend

BIN_DIR=../bin

# Check for double precision
ifeq ($(DOUBLE_PRECISION), ON)
CFLAGS += -DDOUBLE
else
CFLAGS += -DSINGLE
endif

# Set executable name and add includes & libraries for MPI if needed.
CoMD_VARIANT = CoMD-serial
ifeq ($(DO_MPI), ON)
CoMD_VARIANT = CoMD-mpi
INCLUDES += ${MPI_INCLUDE}
CFLAGS += -DDO_MPI
LDFLAGS += ${MPI_LIB}
endif
CoMD_EXE = ${BIN_DIR}/${CoMD_VARIANT}

LDFLAGS += ${C_LIB} ${OTHER_LIB}
CFLAGS  += ${OPTFLAGS} ${INCLUDES} ${OTHER_INCLUDE}


SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:.c=.o)


DEFAULT: ${CoMD_EXE}

%.o: %.c
	${CC} ${CFLAGS} -c $< -o $@

${CoMD_EXE}: ${BIN_DIR} CoMD_info.h ${OBJECTS} 
	${CC} ${CFLAGS} -o ${CoMD_EXE} ${OBJECTS} ${LDFLAGS}

CoMD_info.h: Makefile
	./generate_info_header ${CoMD_VARIANT} "$(CC)" "$(CFLAGS)" "$(LDFLAGS)"

${BIN_DIR}:
	@if [ ! -d ${BIN_DIR} ]; then mkdir -p ${BIN_DIR} ; fi

clean:
	rm -f *.o CoMD_info.h .depend

distclean: clean
	rm -f ${CoMD_EXE} .depend.bak
	rm -rf html latex

.depend: $(SOURCES)
	@touch .depend
	@$(MAKE) --no-print-directory depend

depend:
	@echo "Rebuilding dependencies..."
	@$(MAKE) CoMD_info.h
	@makedepend -f .depend -Y. --$(CFLAGS)-- $(SOURCES) 2> /dev/null


-include .depend
