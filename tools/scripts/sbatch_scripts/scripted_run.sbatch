#!/bin/bash
#SBATCH -p skx-normal # queue
#SBATCH -t 01:59:00   # max time
#SBATCH -A lph_mem

#### Set up env params #############################
module load intel
export MPICH_ASYNC_PROGRESS=1
export OMP_NUM_THREADS=1
####################################################

## Setup parameters ################################
RUN_CMD=ibrun
EXEC_FILE=./lulesh
MAX_RUNS=5
RESULT_PREFIX=time_measure
####################################################

## Application parameters ##########################
APP_INPUTS=(40 60)
iterations=200
####################################################

## Overwrite parameters from run_sbatch_files.sh ##
if [ "$#" -gt 0 ]; then
  ALL_ARGS=( "$@" )
  iterations=${ALL_ARGS[0]}
  unset ALL_ARGS[0]
  APP_INPUTS=( "${ALL_ARGS[@]}" )
  echo "app inputs ${APP_INPUTS[@]}"
fi
####################################################

NUM_MEASURE=$(seq 1 $MAX_RUNS)
for COUNT in ${NUM_MEASURE}
do
  for input in "${APP_INPUTS[@]}"
  do
    echo "(time -p $RUN_CMD $EXEC_FILE -i $iterations -s $input) 2>> ${RESULT_PREFIX}_${NUM_MPI_RANKS}"
  done
done
