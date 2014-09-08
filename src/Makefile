CC=mpic++
CFLAGS=-g -fPIC -Wall -std=gnu++0x -D_WORK=1 -DGONG=1 
LDFLAGS=
SOURCES= $(wildcard *.cc)
OBJECTS=$(SOURCES:.cc=.o)
LIBOUTPUT=../lib/libcds.so

all: $(SOURCES) $(LIBOUTPUT)


.cc.o:
	 @echo "\nCD runtime is being compiled $<"
	 $(CC) -c $(CFLAGS) $< -o $@

#$(LIBOUTPUT): $(OBJECTS) 
#	 #$(CC) $(LDFLAGS) $(OBJECTS) -o $@
#	 ar -cr $@ $(OBJECTS) 


$(LIBOUTPUT): $(OBJECTS) 
	 @echo "\nBuilding .so file for executable \n"
	 ${CC} -shared -Wl,-soname,libcds.so -o ${LIBOUTPUT} ${OBJECTS} ${LDFLAGS}

clean:
	rm $(LIBOUTPUT) $(OBJECTS)


