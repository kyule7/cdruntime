#!/bin/bash
mkdir run_8
mkdir run_64
mkdir run_216
mkdir run_512
mkdir run_1000
execaddr=..
#scriptaddr=/scratch/03341/kyushick/lulesh/0312/run_fg_216
cd run_8; #cp $scriptaddr/run.sbatch run.sbatch
ln -s ../config.yaml
ln -s $execaddr/lulesh_bare
ln -s $execaddr/lulesh_incr
ln -s $execaddr/lulesh_full
ln -s $execaddr/lulesh_cdrt
ln -s $execaddr/lulesh_fgcd
cd ..
cd run_64; #cp $scriptaddr/run.sbatch run.sbatch
ln -s ../config.yaml
ln -s $execaddr/lulesh_bare
ln -s $execaddr/lulesh_incr
ln -s $execaddr/lulesh_full
ln -s $execaddr/lulesh_cdrt
ln -s $execaddr/lulesh_fgcd
cd ..
cd run_216; #cp $scriptaddr/run.sbatch run.sbatch
ln -s ../config.yaml
ln -s $execaddr/lulesh_bare
ln -s $execaddr/lulesh_incr
ln -s $execaddr/lulesh_full
ln -s $execaddr/lulesh_cdrt
ln -s $execaddr/lulesh_fgcd
cd ..
cd run_512; #cp $scriptaddr/run.sbatch run.sbatch
ln -s ../config.yaml
ln -s $execaddr/lulesh_bare
ln -s $execaddr/lulesh_incr
ln -s $execaddr/lulesh_full
ln -s $execaddr/lulesh_cdrt
ln -s $execaddr/lulesh_fgcd
cd ..
cd run_1000; #cp $scriptaddr/run.sbatch run.sbatch
ln -s ../config.yaml
ln -s $execaddr/lulesh_bare
ln -s $execaddr/lulesh_incr
ln -s $execaddr/lulesh_full
ln -s $execaddr/lulesh_cdrt
ln -s $execaddr/lulesh_fgcd
cd ..

