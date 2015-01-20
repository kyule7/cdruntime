
#Uncomment here if you want to build from source code
#g++ -g -std=c++11 -o test_single_thread ./test_single_thread.cc ../../src/cd.cc ../../src/cd_handle.cc ../../src/cd_entry.cc

rm test_single_thread
cd ../../src/
make clean
make DEBUG_VAR=0 SINGLE_VER_VAR=1 PROFILER_ENABLED=1
cd ../test/single_thread
g++ -std=gnu++0x -o test_single_thread -I../../src ./test_single_thread.cc -L../../lib -Wl,-rpath ../../lib -lcds

