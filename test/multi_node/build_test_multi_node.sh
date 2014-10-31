#Uncomment here if you want to build from source code

rm test_multi_node
cd ../../src/
make clean
make MPI_VER_VAR=1 SINGLE_VER_VAR=0 DEBUG_VAR=1
#make MPI_VER_VAR=0 SINGLE_VER_VAR=1
cd ../test/multi_node
mpic++  -std=gnu++0x -o test_multi_node -I../../src -D_MPI_VER=1 -D_SINGLE_VER=0 -Dcomm_log ./test_multi_node.cc -L../../lib -Wl,-rpath ../../lib -lcds
