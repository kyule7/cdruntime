DEBUG_VAR = 0
MPI_VER_VAR = 0
PGAS_VER_VAR = 0
SINGLE_VER_VAR = 1


PROFILE_FLAG = \
      -D_ENABLE_MODULE=0 \
      -D_ENABLE_HIERGRAPH=0 \
      -D_ENABLE_SCOPE=1 \
      -D_ENABLE_GRAPH=0 \
      -D_ENABLE_ATTR=1 \
      -D_ENABLE_COMP=1 \
      -D_ENABLE_CDNODE=0




ifeq ($(MPI_VER_VAR), 0)
CC=g++
else
CC=mpic++
endif
CFLAGS=-g -fPIC -Wall -std=gnu++0x -fmax-errors=2 ${SIGHT_CFLAGS}
LDFLAGS= ${SIGHT_LINKFLAGS}
CDFLAGS=-D_MPI_VER=$(MPI_VER_VAR) -D_PGAS_VER=$(PGAS_VER_VAR) -D_SINGLE_VER=$(SINGLE_VER_VAR) \
        -D_DEBUG=$(DEBUG_VAR)
SOURCES= $(wildcard *.cc)
OBJECTS=$(SOURCES:.cc=.o)
LIBOUTPUT=../lib/libcds.so

all: $(SOURCES) $(LIBOUTPUT)


.cc.o:
#	 @echo "\nCD runtime is being compiled $<"
	 $(CC) -c $(CFLAGS) $(CDFLAGS) $< -o $@

#$(LIBOUTPUT): $(OBJECTS) 
#	 #$(CC) $(LDFLAGS) $(OBJECTS) -o $@
#	 ar -cr $@ $(OBJECTS) 


$(LIBOUTPUT): $(OBJECTS) 
	 @echo "\nBuilding .so file for executable \n"
	 ${CC} -shared -Wl,-soname,libcds.so -o ${LIBOUTPUT} ${OBJECTS} ${LDFLAGS}

clean:
	rm $(LIBOUTPUT) $(OBJECTS)


