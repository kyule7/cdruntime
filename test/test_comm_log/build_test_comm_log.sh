
#Uncomment here if you want to build from source code
#g++ -g -std=c++11 -o test_single_thread ./test_single_thread.cc ../../src/cd.cc ../../src/cd_handle.cc ../../src/cd_entry.cc

rm test_comm_log
cd ../../src/
make clean
make #-f Makefile.debug
cd ../test/test_comm_log/
g++  -std=gnu++0x -o test_comm_log -I../../src ./test_comm_log.cc -L../../lib -lcds

