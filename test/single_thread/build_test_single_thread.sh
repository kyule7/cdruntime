
#Uncomment here if you want to build from source code
#g++ -g -std=c++11 -o test_single_thread ./test_single_thread.cc ../../src/cd.cc ../../src/cd_handle.cc ../../src/cd_entry.cc


rm test_single_thread
cd ../../src/
make clean
make
cd ../test/single_thread
mpic++  -std=gnu++0x -o test_single_thread ./test_single_thread.cc -L../../lib -Wl,-rpath ../../lib -lcds 
