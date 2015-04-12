
#Uncomment here if you want to build from source code
#g++ -g -std=c++11 -o test_single_thread ./test_single_thread.cc ../../src/cd.cc ../../src/cd_handle.cc ../../src/cd_entry.cc

rm test_single_thread
cd ../../src/
make clean
make DEBUG_VAR=1 SINGLE_VER_VAR=1 MPI_VER_VAR=0 PROFILER_ENABLED=0 LOGGING_ENABLED=0
cd ../test/single_thread
mpic++ -std=gnu++0x -o test_single_thread -D_SINGLE_VER=1 -I../../src ./test_single_thread.cc -L../../lib -Wl,-rpath ../../lib -lcds

