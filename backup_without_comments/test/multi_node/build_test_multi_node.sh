#Uncomment here if you want to build from source code

rm test_multi_node
cd ../../src/
make clean
make
cd ../test/multi_node
mpic++  -std=gnu++0x -o test_multi_node -I../../src ./test_multi_node.cc -L../../lib -Wl,-rpath ../../lib -lcds 
