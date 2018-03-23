#!/bin/bash
#SBATCH -J lul             # Job name
#SBATCH -o lul.1000.%j.out # stdout; %j expands to jobid
#SBATCH -e lul.1000.%j.err # stderr; skip to combine stdout and stderr
#SBATCH -p skx-normal      # queue
#SBATCH -N 21              # Number of nodes, not cores (16 cores/node)
#SBATCH -n 1000            # Total number of MPI tasks 
#SBATCH -t 01:59:00        # max time
#SBATCH -A lph_mem

#### Set up env params ##############
module load intel
export MPICH_ASYNC_PROGRESS=1
export OMP_NUM_THREADS=1
#####################################

## Setup parameters #################
RUN_CMD=ibrun
EXEC_FILE=./lulesh
NUM_MPI_RANKS=1000
MAX_RUNS=5
RESULT_PREFIX=time_measure
#####################################

## Application parameters ###########
inputs=(30 40)
ITERATIONS=10
#####################################
touch ${RESULT_PREFIX}_${NUM_MPI_RANKS}

NUM_MEASURE=$(seq 1 $MAX_RUNS)
for COUNT in ${NUM_MEASURE}
do
  for input in "${inputs[@]}"
  do
    echo "(time -p $RUN_CMD $EXEC_FILE -i $ITERATIONS -s $input) 2>> ${RESULT_PREFIX}_${NUM_MPI_RANKS}"
    $RUN_CMD $EXEC_FILE -i $ITERATIONS -s $input
    (time -p $RUN_CMD $EXEC_FILE -i $ITERATIONS -s $input) 2>> ${RESULT_PREFIX}_${NUM_MPI_RANKS}
  done
done
