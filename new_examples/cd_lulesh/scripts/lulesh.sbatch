#!/bin/bash -l
#SBATCH -p debug
#SBATCH -N 2
#SBATCH -C haswell
#SBATCH -L SCRATCH
#SBATCH -J lulesh
#SBATCH -o lulesh.%j.out
#SBATCH -e lulesh.%j.err
#SBATCH -t 00:30:00
#DW jobdw capacity=80GB access_mode=striped type=scratch
#DW stage_out source=$DW_JOB_STRIPED/lulesh_cd16 destination=/global/homes/k/kyushick/Work/CDs/cori/libc/new_examples/cd_lulesh/run_10252017 type=directory
srun -n 16 ./lulesh_cd -i 10 -s 50


