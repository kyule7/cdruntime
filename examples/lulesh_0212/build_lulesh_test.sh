make clean
cd ../../src/
make clean
make DEBUG_VAR=1 MPI_VER_VAR=1 SINGLE_VER_VAR=0
cd ../examples/lulesh
make
#mpirun -np 8 ./lulesh_cd
