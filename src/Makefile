include ../Makefile.extern

DEBUG_VAR = 1
MPI_VER_VAR = 1
PGAS_VER_VAR = 0
SINGLE_VER_VAR = 0

PROFILER_ENABLED=0
LOGGING_ENABLED=1

MEDIUM = -D_MEMORY=1 \
         -D_PFS=0 \
         -D_SSD=0 \
         -D_HDD=0 


ifeq ($(PROFILER_ENABLED), 1)
PROFILER_FLAGS = \
      -D_ENABLE_MODULE=1 \
      -D_ENABLE_HIERGRAPH=0 \
      -D_ENABLE_SCOPE=0 \
      -D_ENABLE_GRAPH=0 \
      -D_ENABLE_ATTR=0 \
      -D_ENABLE_COMP=0 \
      -D_ENABLE_CDNODE=0
endif

MAKE_ARGS = CD_ROOT=${CD_ROOT} SIGHT_ROOT=${SIGHT_ROOT} \
						CD_SRC=${CD_SRC} CD_TOOLS=${CD_TOOLS} CD_LIB=${CD_LIB} CD_AUTOTUNER=${CD_AUTOTUNER} \
						CC=${CC} CFLAGS=${CFLAGS} LDFLAGS=${LDFLAGS} CDS_FLAGS=${CDS_FLAGS} ERR_FLAGS=${ERR_FLAGS} \
						PROFILER_ENABLED=${PROFILER_ENABLED} DEBUG_VAR=${DEBUG_VAR} \
						MPI_VER_VAR=${MPI_VER_VAR} PGAS_VER_VAR=${PGAS_VER_VAR} SINGLE_VER_VAR=${SINGLE_VER_VAR}


ifeq ($(MPI_VER_VAR), 0)
CC=g++
else
CC=mpig++
endif

ERR_FLAGS=-Wall -fmax-errors=3
CFLAGS  = -g -fPIC -std=gnu++0x ${ERR_FLAGS} 
LDFLAGS = #${SIGHT_LINKFLAGS}
CDS_FLAGS=-D_MPI_VER=$(MPI_VER_VAR) -D_PGAS_VER=$(PGAS_VER_VAR) -D_SINGLE_VER=$(SINGLE_VER_VAR) \
        -D_DEBUG=$(DEBUG_VAR) ${PROFILER_FLAGS} \
        -D_KL=1 


ifeq ($(PROFILER_ENABLED), 1)
#LDFLAGS += -L${CD_LIB} -Wl,-rpath ${CD_LIB} -lprofiler 
LDFLAGS += ${SIGHT_LINKFLAGS}
#CFLAGS  += -I$(CD_SRC)/profiler #${SIGHT_CFLAGS}
CFLAGS  += ${SIGHT_CFLAGS}
endif

ifeq ($(PROFILER_ENABLED), 1)

CDS_FLAGS += -D_PROFILER=$(PROFILER_ENABLED)
endif

ifeq ($(LOGGING_ENABLED), 1)
CDS_FLAGS += -Dcomm_log
LDFLAGS += -ldl
endif

CDS_FLAGS += $(MEDIUM)

CDS_SOURCES=$(wildcard *.cc)
CDS_OBJECTS=$(CDS_SOURCES:.cc=.o)
CDS_OUTPUT=${CD_LIB}/libcds.so


all: $(CDS_SOURCES) $(CDS_OUTPUT)  


.cc.o:
	@echo "\nCD runtime is being compiled $<"
	$(CC) -c $(CFLAGS) $(CDS_FLAGS) $< -o $@

$(CDS_OUTPUT): $(CDS_OBJECTS)  
	@echo "\nBuilding .so file for executable \n"
	${CC} -shared -Wl,-soname,libcds.so -o ${CDS_OUTPUT} ${CDS_OBJECTS} ${LDFLAGS}

clean:
	rm $(CDS_OUTPUT) $(CDS_OBJECTS)
#	cd profiler; make clean







#$(PROFILER_OUTPUT): $(PROFILER_CDS_OBJECTS)
#	@echo "\nBuilding profiler \n"
#	${CC} -shared -Wl,-soname,libprofiler.so -o ${PROFILER_OUTPUT} ${PROFILER_OBJECTS} 

#cd_malloc.so:
#	g++ -std=gnu++0x -fPIC -Wall -ldl -Dcomm_log -o cd_malloc.so -shared cd_malloc.cc -L../lib -Wl,-rpath ../lib

#libc_rn.so:
#	g++ -std=gnu++0x -fPIC -Wall -ldl -Dcomm_log -o libc_rn.so -shared libc_rn.cc -L../lib -Wl,-rpath ../lib

#.PHONY: profiler
#ifeq ($(PROFILER_ENABLED), 1)
#profiler: 
#	cd profiler; make ${MAKE_ARGS}
#endif



