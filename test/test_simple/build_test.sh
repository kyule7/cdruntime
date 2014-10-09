rm test_simple
cd ../../src/
make clean
#make DEBUG_VAR=0 SINGLE_VER_VAR=1 MEMORY=0
make 
cd ../test/test_simple
g++ -std=gnu++0x -o test_simple -I../../src ./test_simple.cc -L../../lib -Wl,-rpath ../../lib -lcds

