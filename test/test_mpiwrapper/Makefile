all:
	#mpic++  -std=gnu++0x -o test_p2p -I../../src -D_MPI_VER=1 -D_PGAS_VER=0 -D_SINGLE_VER=0 -D_DEBUG=1 -Dcomm_log -g ./mpi_test_p2p.cc -L../../lib -Wl,-rpath ../../lib -lcds 
	mpic++  -std=gnu++0x -o test_nbp2p -I../../src -D_MPI_VER=1 -D_PGAS_VER=0 -D_SINGLE_VER=0 -D_DEBUG=1 -Dcomm_log -g ./mpi_test_nbp2p.cc -L../../lib -Wl,-rpath ../../lib -lcds 
	#mpic++  -std=gnu++0x -o test_collective -I../../src -D_MPI_VER=1 -D_PGAS_VER=0 -D_SINGLE_VER=0 -D_DEBUG=1 -Dcomm_log -g ./mpi_test_collective.cc -L../../lib -Wl,-rpath ../../lib -lcds 

clean:
	rm -rf tmp.out.* test_*

