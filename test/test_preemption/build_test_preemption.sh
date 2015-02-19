#Uncomment here if you want to build from source code
rm output_*
rm output/output_*
rm test_preemption
cd ../../src/
make clean
make DEBUG_VAR=1 MPI_VER_VAR=1 SINGLE_VER_VAR=0
cd ../test/test_preemption
mpic++  -std=gnu++0x -o test_preemption -I../../src -D_DEBUG=1 -D_MPI_VER=1 -D_SINGLE_VER=0 -D_KL=1 ./test_preemption.cc -L../../lib -Wl,-rpath ../../lib -lcds 
