#!/bin/bash
#SBATCH -J myMPI_43node            # job name
#SBATCH -o myMPI_43node.o%j        # output and error file name (%j expands to jobID)
#SBATCH -N 43                # number of nodes requested
#SBATCH -n 1024               # total number of mpi tasks requested
#SBATCH -p skx-normal       # queue (partition) -- normal, development, etc.
#SBATCH -t 00:20:00         # run time (hh:mm:ss) - 0.5 hours

# Slurm email notifications are now working on Stampede2
#SBATCH --mail-user=yongkee.kwon@gmail.com
#SBATCH --mail-type=begin   # email me when the job starts
#SBATCH --mail-type=end     # email me when the job finishes

ibrun -np 1024 ../CoMD-mpi -i 16 -j 8 -k 8 -x 640 -y 640 -z 640 -1 1 -4 5763 > 1024_43node_1024K.log
