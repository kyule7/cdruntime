SHELL=/bin/sh
CXX=mpic++
PROGRAM=matrix_vector_multiply
$(PROGRAM): matrix_vector_multiply.cc
	$(CXX) -std=gnu++0x -o $@ $< -I../../include -L../../lib -lcd -Wl,-rpath ../../lib 

clean:
	rm $(PROGRAM); rm progress.*; rm reexecute.*; rm escalate.*; rm -rf debug_logs; rm -rf HDD;
