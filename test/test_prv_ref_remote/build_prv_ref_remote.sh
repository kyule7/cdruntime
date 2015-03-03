#Uncomment here if you want to build from source code
rm output_*
rm output/output_*
rm test_prv_ref_remote
cd ../../src/
make clean
make DEBUG_VAR=1 MPI_VER_VAR=1 SINGLE_VER_VAR=0
cd ../test/test_prv_ref_remote
mpic++  -std=gnu++0x -o test_prv_ref_remote -I../../src -D_FIX=1 -D_DEBUG=1 -D_MPI_VER=1 -D_SINGLE_VER=0 -D_KL=1 ./test_prv_ref_remote.cc -L../../lib -Wl,-rpath ../../lib -lcds 
