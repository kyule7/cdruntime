all:
	mpic++  -std=gnu++0x -o hello_world -I../../src -D_MPI_VER=1 -D_SINGLE_VER=0 -Dcomm_log ./mpi_hello_world.cc -L../../lib -Wl,-rpath ../../lib -lcds 

clean:
	rm hello_world tmp.out.*

