#!/bin/bash
#SBATCH -J matmul          # job name
#SBATCH -o matmul.o%j      # output and error file name (%j expands to jobID)
#SBATCH -n 32               # total number of mpi tasks requested
#SBATCH -N 4               # total number of nodes requested
#SBATCH -p development     # queue (partition) -- normal, development, etc.
#SBATCH -t 01:30:00        # run time (hh:mm:ss) - 1.5 hours
#SBATCH --mail-user=kyushick@utexas.edu
#SBATCH --mail-type=begin  # email me when the job starts
#SBATCH --mail-type=end    # email me when the job finishes
#SBATCH -A PCSE-2016        # allocation name

ibrun ./test_mm 128 4 1024 2 0      # run the MPI executable named a.out



