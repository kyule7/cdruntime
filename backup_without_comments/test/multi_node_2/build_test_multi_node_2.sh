#Uncomment here if you want to build from source code

rm test_multi_node_2
cd ../../src/
make clean
make
cd ../test/multi_node_2
mpic++  -std=gnu++0x -o test_multi_node_2 -I../../src ./test_multi_node_2.cc -L../../lib -Wl,-rpath ../../lib -lcds 
