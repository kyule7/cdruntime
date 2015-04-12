CC=mpicc -cc=gcc
CFLAGS=-L../blcr-0.9.pre0/builddir/libcr/.libs -lcr
#CFLAGS= -lcr
LDFLAGS=
SOURCES=mem_excl_test.c crut.c crut_util.c crut_util_libcr.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=mem_excl

all: $(SOURCES) $(EXECUTABLE)
	rm $(OBJECTS)

$(EXECUTABLE): $(OBJECTS) 
	 $(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(CFLAGS)

.cpp.o:
	 $(CC) $(CFLAGS) $< -o $@

clean:
	rm $(EXECUTABLE) $(OBJECTS)

