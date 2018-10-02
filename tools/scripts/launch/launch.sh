#!/bin/bash
cd run_8;
sbatch run.sbatch
cd ..;
cd run_64;
sbatch run.sbatch
cd ..;
cd run_216;
sbatch run.sbatch
cd ..;
cd run_512;
sbatch run.sbatch
cd ..;
cd run_1000;
sbatch run.sbatch
cd ..;
