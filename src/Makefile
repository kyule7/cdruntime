include ../Makefile.extern

DEBUG_VAR = 1
MPI_VER_VAR = 1
PGAS_VER_VAR = 0
SINGLE_VER_VAR = 0

PROFILER_ENABLED=0
LOGGING_ENABLED=1
LIBC_LOGGING_ENABLED=0
ERROR_INJECTION_ENABLED=0

TACC=0

GCC_VERSION_LOWER=1
          

DEBUG_FLAGS = -D_DEBUG=$(DEBUG_VAR) -D_CD_DEBUG=$(DEBUG_VAR) -D_PRV_FILE_NOT_ERASED=0

ifeq ($(PROFILER_ENABLED), 1)
PROFILER_FLAGS =\
      -D_ENABLE_MODULE=0 \
      -D_ENABLE_HIERGRAPH=0 \
      -D_ENABLE_SCOPE=1 \
      -D_ENABLE_ATTR=0 \
      -D_ENABLE_COMP=1 \
      -D_ENABLE_GRAPH=0 
endif

MAKE_ARGS = CD_ROOT=${CD_ROOT} SIGHT_ROOT=${SIGHT_ROOT} \
						CD_SRC=${CD_SRC} CD_TOOLS=${CD_TOOLS} CD_LIB=${CD_LIB} CD_AUTOTUNER=${CD_AUTOTUNER} \
						CC=${CC} CFLAGS=${CFLAGS} LDFLAGS=${LDFLAGS} CDS_FLAGS=${CDS_FLAGS} ERR_FLAGS=${ERR_FLAGS} \
						PROFILER_ENABLED=${PROFILER_ENABLED} DEBUG_VAR=${DEBUG_VAR} \
						MPI_VER_VAR=${MPI_VER_VAR} PGAS_VER_VAR=${PGAS_VER_VAR} SINGLE_VER_VAR=${SINGLE_VER_VAR} 


ifeq ($(MPI_VER_VAR), 0)
CC=g++
LOGGING_ENABLED=0
else
CC=mpic++
endif

ifeq ($(SINGLE_VER_VAR), 1)
CC=g++
LOGGING_ENABLED=0
endif

ERR_FLAGS=-Wall 
ifeq ($(GCC_VERSION_LOWER), 0)
ERR_FLAGS+=-fmax-errors=5
endif

CFLAGS  = -g -fPIC -std=gnu++0x ${ERR_FLAGS}
LDFLAGS = #${SIGHT_LINKFLAGS}

ifeq ($(TACC), 1)
CFLAGS += -xhost -O2
endif

CDS_FLAGS=-D_MPI_VER=$(MPI_VER_VAR) -D_PGAS_VER=$(PGAS_VER_VAR) -D_SINGLE_VER=$(SINGLE_VER_VAR) \
        ${DEBUG_FLAGS}\
        ${PROFILER_FLAGS}


ifeq ($(PROFILER_ENABLED), 1)
CFLAGS  += ${SIGHT_CFLAGS}
LDFLAGS += ${SIGHT_LINKFLAGS}
#CFLAGS  += -I$(CD_SRC)/profiler #${SIGHT_CFLAGS}
#LDFLAGS += -L${CD_LIB} -Wl,-rpath ${CD_LIB} -lprofiler 
endif

ifeq ($(PROFILER_ENABLED), 1)

CDS_FLAGS += -D_PROFILER=$(PROFILER_ENABLED)
endif

ifeq ($(LOGGING_ENABLED), 1)
CDS_FLAGS += -Dcomm_log
LDFLAGS += -ldl
endif

ifeq ($(LIBC_LOGGING_ENABLED), 1)
CDS_FLAGS += -Dlibc_log
endif

ifeq ($(ERROR_INJECTION_ENABLED), 1)
CDS_FLAGS += -D_ERROR_INJECTION_ENABLED=1
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
	rm -rf $(CDS_OUTPUT) $(CDS_OBJECTS)
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



