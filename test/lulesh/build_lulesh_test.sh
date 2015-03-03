make clean
cd ../../src/
make clean
make DEBUG_VAR=1 MPI_VER_VAR=1 SINGLE_VER_VAR=0
cd ../test/lulesh
make
#mpirun -np 8 ./lulesh_cd
