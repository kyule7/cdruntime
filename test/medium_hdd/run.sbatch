#!/bin/bash
#SBATCH -J brian          # job name
#SBATCH -o brian.o%j      # output and error file name (%j expands to jobID)
#SBATCH -n 1               # total number of mpi tasks requested
#SBATCH -N 1               # total number of nodes requested
#SBATCH -p development     # queue (partition) -- normal, development, etc.
#SBATCH -t 01:30:00        # run time (hh:mm:ss) - 1.5 hours
#SBATCH --mail-user=kyushick@utexas.edu
#SBATCH --mail-type=begin  # email me when the job starts
#SBATCH --mail-type=end    # email me when the job finishes
#SBATCH -A lph_mem        # allocation name

ibrun ./medium_hdd      # run the MPI executable named a.out



