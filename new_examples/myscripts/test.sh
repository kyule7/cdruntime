#!/bin/bash
if [ "$#" -ne 2 ]; then
  echo "Illegal number of parameters"
  exit
fi
EXEC_BIN=lulesh_nocd
HPCSTRUCT_FILENAME=${EXEC_BIN}.hpcstruct
SBATCH_FILENAME=lulesh.sbatch
