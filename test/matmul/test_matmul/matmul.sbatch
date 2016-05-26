#!/bin/bash
#SBATCH -J mm_mpi16          # job name
#SBATCH -o mm_mpi16.o%j      # output and error file name (%j expands to jobID)
#SBATCH -n 16               # total number of mpi tasks requested
#SBATCH -N 1               # total number of nodes requested
#SBATCH -p normal     # queue (partition) -- normal, development, etc.
#SBATCH -t 00:40:00        # run time (hh:mm:ss) - 1.5 hours
#SBATCH --mail-user=kyushick@utexas.edu
#SBATCH --mail-type=begin  # email me when the job starts
#SBATCH --mail-type=end    # email me when the job finishes
#SBATCH -A lph_mem        # allocation name
export OMP_NUM_THREADS=1
export OMP_NESTED=1

ibrun tacc_affinity ./test_mm_mpi 16 1 2048 2 0 >> omp_result_mpi_weak.out

