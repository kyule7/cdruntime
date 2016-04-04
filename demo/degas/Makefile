SHELL=/bin/sh
CXX=mpic++
TUNED=tuned
GLOBAL=global

all: $(TUNED) $(GLOBAL)

$(GLOBAL): matrix_vector_multiply.cc
	$(CXX) -std=gnu++0x -o $@ $< -I../../include -L../../lib -lcd -Wl,-rpath ../../lib 

$(TUNED): matrix_vector_multiply.cc
	$(CXX) -std=gnu++0x -D_TUNED -o $@ $< -I../../include -L../../lib -lcd -Wl,-rpath ../../lib 

clean:
	rm $(TUNED); rm $(GLOBAL); rm global.*; rm tuned.*; rm reexecute.*; rm escalate.*; rm -rf debug_logs; rm -rf HDD;

clean_errors:
	rm escalate.*; rm reexecute.*;
