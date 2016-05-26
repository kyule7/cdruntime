CC=mpicxx
CFLAGS=-std=gnu++0x -O2 -g -D_DEBUG=0 -D_MPI=1 -openmp -I.
all: test_mm test_single
test_mm: test_mm.cc matmul.h matmul_seq.h matmul_omp.h matmul_mpi.h util.h
	$(CC) $(CFLAGS) test_mm.cc -o ../bin/test_mm -lm

test_single: test_single.cc matmul.h matmul_seq.h matmul_omp.h matmul_mpi.h util.h
	$(CC) $(CFLAGS) test_single.cc -o ../bin/test_omp -lm
clean:
	rm *~; rm ../bin/test_mm; rm dbg_*
