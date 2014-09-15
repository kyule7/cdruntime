CC=mpic++
CFLAGS=-g -fPIC -Wall -std=gnu++0x 
LDFLAGS=
CDFLAGS=-D_MPI_VER=1 -D_PGAS_VER=0 -D_SINGLE_VER=0 -D_WORK=1 -DGONG=1
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


