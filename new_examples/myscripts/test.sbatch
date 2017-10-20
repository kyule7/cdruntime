#!/bin/bash
#SBATCH -J lulesh8          # job name
#SBATCH -o lulesh8.o%j      # output and error file name (%j expands to jobID)
#SBATCH -n 8               # total number of mpi tasks requested
#SBATCH -N 2               # total number of nodes requested
#SBATCH -p normal     # queue (partition) -- normal, development, etc.
#SBATCH -t 01:40:00        # run time (hh:mm:ss) - 1.5 hours
#SBATCH --mail-user=kyushick@utexas.edu
#SBATCH --mail-type=begin  # email me when the job starts
#SBATCH --mail-type=end    # email me when the job finishes
#SBATCH -A lph_mem        # allocation name

echo "SLURM_JOBID: " $SLURM_JOBID
echo "SLURM_ARRAY_TASK_ID: " $SLURM_ARRAY_TASK_ID
echo "SLURM_ARRAY_JOB_ID: " $SLURM_ARRAY_JOB_ID
echo "LULESH_INPUT_SIZE: $LULESH_INPUT_SIZE"
ibrun hpcrun --event WALLCLOCK@400000 --event PAPI_FDV_INS@10000 ./lulesh_nocd -i 100 -s $LULESH_INPUT_SIZE
#ibrun hpcrun --event WALLCLOCK@400000 --event PAPI_FDV_INS@10000 ./lulesh_nocd -i 100 -s 40
#ibrun hpcrun --event WALLCLOCK@400000 --event PAPI_FDV_INS@10000 ./lulesh_nocd -i 100 -s 60
#ibrun hpcrun --event WALLCLOCK@400000 --event PAPI_FDV_INS@10000 ./lulesh_nocd -i 100 -s 80
#ibrun hpcrun --event WALLCLOCK@400000 --event PAPI_FDV_INS@10000 ./lulesh_nocd -i 100 -s 100
