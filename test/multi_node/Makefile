all:
	mpic++  -std=gnu++0x -o test_multi_node -I../../src -D_MPI_VER=1 -D_DEBUG=1 -D_ERROR_INJECTION_ENABLED=1 -D_LOGGING_ENABLED=1 -g ./test_multi_node.cc -L../../lib -Wl,-rpath ../../lib -lcds 
	#mpic++  -std=gnu++0x -o test_comm_log -I../../src -D_MPI_VER=1 -D_SINGLE_VER=0 -Dcomm_log ./test_comm_log.cc -L../../lib -Wl,-rpath ../../lib -lcds 
#	mpic++  -std=gnu++0x -o test_comm_log -I../../src -D_MPI_VER=1 -D_DEBUG=1 -D_KL=1 -D_SINGLE_VER=0 -Dcomm_log -g ./test_comm_log.cc -L../../lib -Wl,-rpath ../../lib -lcds 
clean:
	rm -rf test_comm_log tmp.out.*

