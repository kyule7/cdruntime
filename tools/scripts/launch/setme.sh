#!/bin/bash
unset CD_NO_PRESERVE
unset CD_NO_OPERATE
unset CD_NO_ERROR
export MPICH_ASYNC_PROGRESS=1
export I_MPICH_ASYNC_PROGRESS=1
export MPI_THREAD_MULTIPLE=1
export KEEP_TOTAL_FAILURE_RATE_SAME=1
export CD_EXEC_NAME=synthetic
export CD_EXEC_DETAILS=baseline
export CD_EXEC_ITERS=1
export LULESH_LV0=9
export LULESH_LV1=3
export LULESH_LV2=1
export CD_DATA_GROW_UNIT=1048576
export CD_MAX_LEVEL_PRINT=5
export IBRUN_TASKS_PER_NODE=48
#export CD_NO_PRESERVE=1
#export CD_NO_OPERATE=1
export CD_NO_ERROR=1

if [ "$#" -gt 0 ]; then
  if [[ -n "$1" ]]; then
    echo "export CD_EXEC_NAME=$1"
    export CD_EXEC_NAME=$1
  fi
  if [[ -n "$2" ]]; then
    echo "export LULESH_LV0=$2"
    export LULESH_LV0=$2
  fi
  if [[ -n "$3" ]]; then
    echo "export LULESH_LV1=$3"
    export LULESH_LV1=$3
  fi
  if [[ -n "$4" ]]; then
    echo "export LULESH_LV2=$4"
    export LULESH_LV2=$4
  fi
  if [[ -n "$5" ]]; then
    echo "export CD_EXEC_DETAILS=$5"
    export CD_EXEC_DETAILS=$5
  fi
  if [[ -n "$6" ]]; then
    echo "export CD_EXEC_ITERS=$6"
    export CD_EXEC_ITERS=$6
  fi
  if [[ -n "$7" ]]; then
    echo "export CD_NO_PRESERVE=1"
    export CD_NO_PRESERVE=1
    export KEEP_TOTAL_FAILURE_RATE_SAME=0
  fi
fi
echo "ibrun ./$CD_EXEC_NAME -i $CD_EXEC_ITERS -s $CD_EXEC_DETAILS"
#ibrun  -n 125 -o 0  ./$CD_EXEC_NAME -i $CD_EXEC_ITERS -s $CD_EXEC_DETAILS

