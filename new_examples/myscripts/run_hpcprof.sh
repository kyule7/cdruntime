#!/bin/bash

EXEC_BIN=lulesh_nocd
HPCSTRUCT_FILENAME=${EXEC_BIN}.hpcstruct
HPC_MEASUREMENT_FILE=$(ls -d hpctoolkit-${EXEC_BIN}-measurements-*)
echo "hpcprof -S ./$HPCSTRUCT_FILENAME -I ./'*' ./$HPC_MEASUREMENT_FILE"
hpcprof -S ./$HPCSTRUCT_FILENAME -I ./'*' ./$HPC_MEASUREMENT_FILE
