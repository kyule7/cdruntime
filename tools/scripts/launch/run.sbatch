#!/bin/bash
#SBATCH -J syn        # Job name
#SBATCH -o syn.1000.%j.out # stdout; %j expands to jobid
#SBATCH -e syn.1000.%j.err # stderr; skip to combine stdout and stderr
#SBATCH -p skx-normal       # queue
#SBATCH -N 42           # Number of nodes, not cores (16 cores/node)
#SBATCH -n 1000          # Total number of MPI tasks 
#SBATCH --begin=now
#SBATCH -t 05:59:00     # max time
#SBATCH -A lph_mem
module load intel
CD_EXEC=./synthesize

#### Set up Sweep Params ###############
export MPICH_ASYNC_PROGRESS=1
export I_MPICH_ASYNC_PROGRESS=1
export CD_EXEC_ITERS=0
export CD_EXEC_DETAILS=0
export KEEP_TOTAL_FAILURE_RATE_SAME=0
#########################################
MAX_RUNS=1
NUM_MEASURE=$(seq 1 $MAX_RUNS)
#########################################

for COUNT in ${NUM_MEASURE}
do
./pasteit.sh
done
