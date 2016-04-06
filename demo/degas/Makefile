SHELL=/bin/sh
CXX=mpic++
LOCAL=local
GLOBAL=global

all: $(LOCAL) $(GLOBAL)

$(GLOBAL): matrix_vector_multiply.cc
	$(CXX) -std=gnu++0x -o $@ $< -I../../include -L../../lib -lcd -Wl,-rpath ../../lib 

$(LOCAL): matrix_vector_multiply.cc
	$(CXX) -std=gnu++0x -D_LOCAL -o $@ $< -I../../include -L../../lib -lcd -Wl,-rpath ../../lib 

clean:
	rm $(LOCAL); rm $(GLOBAL); rm global.*; rm local.*; rm reexecute.*; rm escalate.*; rm -rf debug_logs; rm -rf HDD;

clean_errors:
	rm escalate.*; rm reexecute.*;
