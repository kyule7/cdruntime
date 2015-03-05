#Uncomment here if you want to build from source code

cd ../../src/
make clean
make MPI_VER_VAR=1 SINGLE_VER_VAR=0 DEBUG_VAR=1 LOGGING_ENABLED=1
cd ../test/test_mpiwrapper
make clean
make
