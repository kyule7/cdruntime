# Declaration of variables
CDPATH := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))/../..
CC = mpicxx
# LIBS = -static -Ljsoncpp/libs -ljson
#LIBS = -static

CD_CFLAGS = -std=gnu++0x -fmax-errors=1 -fPIC -I${CDPATH}/include -I${CDPATH}/src -I${CDPATH}/src/persistence -I${CDPATH}/plugin/wrapper
#WRAPPER_LFLAGS = -lwrapLibc -L${CDPATH}/plugin/wrapper -Wl,-rpath=${CDPATH}/plugin/wrapper -lc 
WRAPPER_LFLAGS = -lc -lwrapLibc -L${CDPATH}/plugin/wrapper -Wl,-rpath=${CDPATH}/plugin/wrapper  
CD_LFLAGS = -L${CDPATH}/lib -lcd -Wl,-rpath=${CDPATH}/lib -L${CDPATH}/src/persistence -lpacker -Wl,-rpath=${CDPATH}/src/persistence -ldl
CD_LINKFLAGS = ${WRAPPER_LFLAGS} ${CD_LFLAGS}

CC_FLAGS = $(CD_CFLAGS) -g -Ijsoncpp/include -Impreal -Wno-deprecated-declarations -fpermissive
LFLAGS = -static-libstdc++  $(CD_LINKFLAGS)

# File names
EXEC = synthesize
SOURCES = $(wildcard *.cc)
OBJECTS = $(SOURCES:.cc=.o)
 
# Main target
$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXEC) $(LIBS) libjson.a $(LFLAGS)
 
# To obtain object files
%.o: %.cc
	$(CC) -c $(CC_FLAGS) $< -o $@ $(LFLAGS)
 
# To remove generated files
clean:
	rm -f $(EXEC) $(OBJECTS) #libjson.a

