#!/bin/bash
#SBATCH -J lulesh64          # job name
#SBATCH -o lulesh64.o%j      # output and error file name (%j expands to jobID)
#SBATCH -n 64               # total number of mpi tasks requested
#SBATCH -N 4               # total number of nodes requested
#SBATCH -p normal     # queue (partition) -- normal, development, etc.
#SBATCH -t 00:40:00        # run time (hh:mm:ss) - 1.5 hours
#SBATCH --mail-user=kyushick@utexas.edu
#SBATCH --mail-type=begin  # email me when the job starts
#SBATCH --mail-type=end    # email me when the job finishes
#SBATCH -A lph_mem        # allocation name

ibrun ../lulesh_cd -i 200 >> result_lulesh.txt 
