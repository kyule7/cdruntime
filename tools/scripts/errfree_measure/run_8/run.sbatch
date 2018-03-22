#!/bin/bash
#SBATCH -J lulf           # Job name
#SBATCH -o lulf.8.%j.out  # stdout; %j expands to jobid
#SBATCH -e lulf.8.%j.err  # stderr; skip to combine stdout and stderr
#SBATCH -p skx-normal     # queue
#SBATCH -N 1              # Number of nodes, not cores (16 cores/node)
#SBATCH -n 8              # Total number of MPI tasks 
#SBATCH --begin=now
#SBATCH -t 05:59:00       # max time
#SBATCH -A lph_mem
module load intel
#### Set up interval ###############
lv0_intvl_s40=4
lv1_intvl_s40=0
lv2_intvl_s40=0
lv0_intvl_s60=4
lv1_intvl_s60=0
lv2_intvl_s60=0
lv0_intvl_s80=4
lv1_intvl_s80=0
lv2_intvl_s80=0

#### Set up Sweep Params ###############
iterations=400
export MPICH_ASYNC_PROGRESS=1
export OMP_NUM_THREADS=1
export CD_EXEC_ITERS=$iterations
########################################

LULESH_EXEC=./lulesh_cdrt

#### Set up Sweep Params ###############
export KEEP_TOTAL_FAILURE_RATE_SAME=0
export CD_EXEC_NAME=lulesh_errfree
########################################
MAX_RUNS=3
NUM_MEASURE=$(seq 1 $MAX_RUNS)
for COUNT in ${NUM_MEASURE}
do
  export CD_EXEC_DETAILS=40
  export LULESH_LV0=$lv0_intvl_s40
  export LULESH_LV1=$lv1_intvl_s40
  export LULESH_LV2=$lv2_intvl_s40
  echo "ibrun $LULESH_EXEC -i $iterations -s 40 $lv0_intvl_s40 $lv1_intvl_s40 $lv2_intvl_s40"
  ibrun $LULESH_EXEC -i $iterations -s 40
  export CD_EXEC_DETAILS=60
  export LULESH_LV0=$lv0_intvl_s60
  export LULESH_LV1=$lv1_intvl_s60
  export LULESH_LV2=$lv2_intvl_s60
  echo "ibrun $LULESH_EXEC -i $iterations -s 60 $lv0_intvl_s60 $lv1_intvl_s60 $lv2_intvl_s60"
  ibrun $LULESH_EXEC -i $iterations -s 60
  export CD_EXEC_DETAILS=80
  export LULESH_LV0=$lv0_intvl_s80
  export LULESH_LV1=$lv1_intvl_s80
  export LULESH_LV2=$lv2_intvl_s80
  echo "ibrun $LULESH_EXEC -i $iterations -s 80 $lv0_intvl_s80 $lv1_intvl_s80 $lv2_intvl_s80"
  ibrun $LULESH_EXEC -i $iterations -s 80
#########################################

#### Set up Sweep Params ###############
export CD_NO_PRESERVE=1
export CD_EXEC_NAME=lulesh_noprv
########################################
  export CD_EXEC_DETAILS=40
  export LULESH_LV0=$lv0_intvl_s40
  export LULESH_LV1=$lv1_intvl_s40
  export LULESH_LV2=$lv2_intvl_s40
  echo "ibrun $LULESH_EXEC -i $iterations -s 40 $lv0_intvl_s40 $lv1_intvl_s40 $lv2_intvl_s40"
  ibrun $LULESH_EXEC -i $iterations -s 40
  export CD_EXEC_DETAILS=60
  export LULESH_LV0=$lv0_intvl_s60
  export LULESH_LV1=$lv1_intvl_s60
  export LULESH_LV2=$lv2_intvl_s60
  echo "ibrun $LULESH_EXEC -i $iterations -s 60 $lv0_intvl_s60 $lv1_intvl_s60 $lv2_intvl_s60"
  ibrun $LULESH_EXEC -i $iterations -s 60
  export CD_EXEC_DETAILS=80
  export LULESH_LV0=$lv0_intvl_s80
  export LULESH_LV1=$lv1_intvl_s80
  export LULESH_LV2=$lv2_intvl_s80
  echo "ibrun $LULESH_EXEC -i $iterations -s 80 $lv0_intvl_s80 $lv1_intvl_s80 $lv2_intvl_s80"
  ibrun $LULESH_EXEC -i $iterations -s 80
#########################################
unset CD_NO_PRESERVE
done
