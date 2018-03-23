#!/bin/bash
#SBATCH -p skx-dev # queue
#SBATCH -t 01:59:00   # max time
#SBATCH -A lph_mem


#### README ########################################
# This script is intended to be launched by 
# run_sbatch_files.sh file. 
####################################################

#### Set up env params #############################
module load intel
#export MPICH_ASYNC_PROGRESS=1
#export OMP_NUM_THREADS=1
####################################################

## Setup parameters ################################
RUN_CMD=ibrun
EXEC_FILE=./lulesh
MAX_RUNS=1
RESULT_PREFIX=time_measure
####################################################

## Application parameters ##########################
APP_INPUTS=(40 60)
ITERATIONS=20
####################################################

## Overwrite parameters from run_sbatch_files.sh ##
if [ "$#" -gt 0 ]; then
  ALL_ARGS=( "$@" )
  EXEC_FILE=${ALL_ARGS[0]}
  ITERATIONS=${ALL_ARGS[1]}
  MAX_RUNS=${ALL_ARGS[2]}
  unset ALL_ARGS[0]
  unset ALL_ARGS[1]
  unset ALL_ARGS[2]
  APP_INPUTS=( "${ALL_ARGS[@]}" )
  echo "Overwitten values -> EXEC_FILE:$EXEC_FILE, ITERATIONS:$ITERATIONS, INPUTS: ${APP_INPUTS[@]}"
fi
####################################################

NUM_MEASURE=$(seq 1 $MAX_RUNS)
for COUNT in ${NUM_MEASURE}
do
  for input in "${APP_INPUTS[@]}"
  do
    echo "$RUN_CMD $EXEC_FILE -i $ITERATIONS -s $input" >> ${RESULT_PREFIX}_${NUM_MPI_RANKS}
    (time -p $RUN_CMD $EXEC_FILE -i $ITERATIONS -s $input) 2>> ${RESULT_PREFIX}_${NUM_MPI_RANKS}
  done
done
