#Uncomment here if you want to build from source code

rm test_multi_node
cd ../../src/
make clean
#<<<<<<< HEAD
#make MPI_VER_VAR=1 SINGLE_VER_VAR=0 DEBUG_VAR=0
##make MPI_VER_VAR=0 SINGLE_VER_VAR=1
#cd ../test/multi_node
#mpic++  -std=gnu++0x -o test_multi_node -I../../src -D_MPI_VER=1 -D_SINGLE_VER=0 -Dcomm_log ./test_multi_node.cc -L../../lib -Wl,-rpath ../../lib -lcds
#=======



make DEBUG_VAR=0 MPI_VER_VAR=1 SINGLE_VER_VAR=0 LOGGING_ENABLED=0
#make MPI_VER_VAR=0 SINGLE_VER_VAR=1
cd ../test/multi_node
mpic++  -std=gnu++0x -o test_multi_node -I../../src -D_KL=1 -D_DEBUG=0 -D_MPI_VER=1 -D_SINGLE_VER=0 ./test_multi_node.cc -L../../lib -Wl,-rpath ../../lib -lcds 
