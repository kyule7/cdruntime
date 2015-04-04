CC=g++
CFLAGS=-g -fPIC -std=gnu++0x 
ERR_FLAGS=-Wall -fmax-errors=5

LDFLAGS=
#SOURCES= cd_interface.cc deserializer.cc config_obj.cc# deserializer.cc
SOURCES=$(wildcard *.cc)
OBJECTS=$(SOURCES:.cc=.o)
EXECUTABLE=interface

all: $(SOURCES) $(EXECUTABLE) 

.cc.o:
	$(CC) -c $(ERR_FLAGS) $(CFLAGS) $< -o $@

$(EXECUTABLE): $(OBJECTS) 
	@echo "\ninterface is being compiled $<"
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@


clean:
	rm $(EXECUTABLE) $(OBJECTS)

