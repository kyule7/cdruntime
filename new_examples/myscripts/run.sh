#!/bin/bash
if [ "$#" -ne 2 ]; then
  echo "Illegal number of parameters"
  exit
fi
APP_NAME=lulesh
EXEC_BIN=${APP_NAME}_nocd
HPCSTRUCT_FILENAME=${EXEC_BIN}.hpcstruct
SBATCH_FILENAME=${APP_NAME}.sbatch
LULESH_TASK_SIZE=$1
LULESH_NODE_SIZE=$(( $LULESH_TASK_SIZE/8 ))
LULESH_INPUT_SIZE=$2
LULESH_FILE_OUT=${APP_NAME}_${1}_${2}.out
LULESH_EXEC_DIR=${APP_NAME}_${1}_${2}
LULESH_JOB_NAME=lu${1}_${2}
echo "mkdir $LULESH_EXEC_DIR; cd $LULESH_EXEC_DIR;"
echo "ln -s ../$EXEC_BIN"
echo "ln -s ../$HPCSTRUCT_FILENAME ."
echo "ln -s ../$SBATCH_FILENAME ."
echo "sbatch --ntasks=$LULESH_TASK_SIZE --nodes=$LULESH_NODE_SIZE --output=$LULESH_FILE_OUT --job-name=$LULESH_JOB_NAME $SBATCH_FILENAME -s $LULESH_INPUT_SIZE"

#mkdir $LULESH_EXEC_DIR; cd $LULESH_EXEC_DIR;
#ln -s ../$EXEC_BIN
#ln -s ../$HPCSTRUCT_FILENAME .
#ln -s ../$SBATCH_FILENAME .
#
#export LULESH_INPUT_SIZE
#sbatch --ntasks=$LULESH_TASK_SIZE --nodes=$LULESH_NODE_SIZE --output=$LULESH_FILE_OUT --job-name=$LULESH_JOB_NAME $SBATCH_FILENAME
