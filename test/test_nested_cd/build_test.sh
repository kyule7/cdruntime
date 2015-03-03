rm test_nested
cd ../../src/
make clean
make DEBUG_VAR=0 SINGLE_VER_VAR=1 MEMORY=0
#make  
cd ../test/test_nested_cd
g++ -std=gnu++0x -o test_nested  -I../../src ./test_nested.cc -L../../lib -Wl,-rpath ../../lib -lcds

