#!/bin/bash
EXECFILE=run.sbatch
cd run_8
sbatch $EXECFILE
cd ..
cd run_64
sbatch $EXECFILE
cd ..
cd run_216
sbatch $EXECFILE
cd ..
cd run_512
sbatch $EXECFILE
cd ..
cd run_1000
sbatch $EXECFILE
cd ..
