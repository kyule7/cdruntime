#!/bin/bash
# rm lulesh_cd
export CKPT_INTERVAL=1
mkdir result_1
cd result_1
cp ../lulesh.sbatch .
sbatch lulesh.sbatch

cd ..
# rm lulesh_cd
export CKPT_INTERVAL=2
mkdir result_2
cd result_2
cp ../lulesh.sbatch .
sbatch lulesh.sbatch

cd ..
# rm lulesh_cd
export CKPT_INTERVAL=4
mkdir result_4
cd result_4
cp ../lulesh.sbatch .
sbatch lulesh.sbatch

cd ..
# rm lulesh_cd
export CKPT_INTERVAL=8
mkdir result_8
cd result_8
cp ../lulesh.sbatch .
sbatch lulesh.sbatch

cd ..
# rm lulesh_cd
export CKPT_INTERVAL=16
mkdir result_16
cd result_16
cp ../lulesh.sbatch .
sbatch lulesh.sbatch

cd ..
# rm lulesh_cd
export CKPT_INTERVAL=32
mkdir result_32
cd result_32
cp ../lulesh.sbatch .
sbatch lulesh.sbatch

cd ..
# rm lulesh_cd
export CKPT_INTERVAL=64
mkdir result_64
cd result_64
cp ../lulesh.sbatch .
sbatch lulesh.sbatch

cd ..
# rm lulesh_cd
export CKPT_INTERVAL=128
mkdir result_128
cd result_128
cp ../lulesh.sbatch .
sbatch lulesh.sbatch

cd ..
#rm lulesh_cd
#make clean; make CD_ENABLED=0
#export CKPT_INTERVAL=4
mkdir result_nocd
cd result_nocd
cp ../lulesh_nocd.sbatch .
sbatch lulesh_nocd.sbatch
