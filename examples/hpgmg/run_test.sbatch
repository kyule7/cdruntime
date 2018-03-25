#!/bin/bash
#SBATCH -J hpgmg-fe          # Job name
#SBATCH -o hpgmg-fe.4.%j.out # stdout; %j expands to jobid
#SBATCH -e hpgmg-fe.4.%j.err # stderr; skip to combine stdout and stderr
#SBATCH -p development  # queue
#SBATCH -N 1            # Number of nodes, not cores (16 cores/node)
#SBATCH -n 4            # Total number of MPI tasks 
#SBATCH -t 00:30:00     # max time
#SBATCH -A lph_mem

#### Set up env params ##############
module load intel
module load hpctoolkit
export MPICH_ASYNC_PROGRESS=1
export OMP_NUM_THREADS=1
#####################################

## Setup parameters #################
MAX_RUNS=3
HPCRUN= #hpcrun
#####################################

set -x
NUM_MEASURE=$(seq 1 $MAX_RUNS)
for COUNT in ${NUM_MEASURE}
do
    ibrun -n 4 $HPCRUN ./bin/hpgmg-fe sample -local 50,10000 -repeat 3 -op_type poisson2
done
set +x
