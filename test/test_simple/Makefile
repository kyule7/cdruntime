


#rm test_simple
#cd ../../src/
#make clean
#make DEBUG_VAR=1 MPI_VER_VAR=1 MEMORY=0
##make 
#cd ../test/test_simple
#g++ -std=gnu++0x -o test_simple -I../../src ./test_simple.cc -D_MPI_VER=1 -D_DEBUG=1 -L../../lib -Wl,-rpath ../../lib -lcds
#



all:
	mpic++  -std=gnu++0x -o test_simple -I../../src -D_ERROR_INJECTION_ENABLED=1 -D_DEBUG=1 -D_MPI_VER=1 -D_SINGLE_VER=0 -D_KL=1 ./test_simple.cc -L../../lib -Wl,-rpath ../../lib -lcds 

clean:
	rm -rf  test_simple 


refresh:
	cd ../../src; make clean; make DEBUG_VAR=1 MPI_VER_VAR=1 SINGLE_VER_VAR=0 LOGGING_ENABLED=0; ERROR_INJECTION_ENABLED=1
