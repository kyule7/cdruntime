#!/bin/bash
CD_SYNTH_PATH=/home1/05973/jeageun/Work/FA2018/CD/stampede2/tools/synthesizer/synthesize
CD_LULESH_PATH=/home1/05973/jeageun/Work/FA2018/CD/stampede2/new_examples/cd_lulesh/lulesh_fgcd

cp $CD_SYNTH_PATH .
cp $CD_LULESH_PATH .
mkdir node_1;
mkdir node_2;
mkdir node_4;
mkdir node_8;
mkdir node_16;
mkdir node_32;
mkdir node_64;

cd node_1;
../get_runfiles.sh
cd ..;

cd node_2;
../get_runfiles.sh
cd ..;

cd node_4;

../get_runfiles.sh

cd ..;

cd node_8;

../get_runfiles.sh
cd ..;

cd node_16;

../get_runfiles.sh
cd ..;

cd node_32;

../get_runfiles.sh
cd ..;

cd node_64;

../get_runfiles.sh
cd ..;

