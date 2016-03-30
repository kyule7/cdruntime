SHELL=/bin/sh
CXX=CC
PROGRAM=matrix_vector_multiply
$(PROGRAM): matrix_vector_multiply.cc
	$(CXX) -o $@ -std=gnu++0x -I../../include $< -L../../lib -lcd  

clean:
	rm $(PROGRAM); rm progress.*; rm reexecute.*; rm escalate.*; rm -rf debug_logs; rm -rf HDD;
