#!/bin/bash
CDTOOL_PATH=~/Work/FA2018/CD/stampede2/tools/scripts/launch
ln -s $CDTOOL_PATH/pasteit.sh .
ln -s $CDTOOL_PATH/prsvHeavy.json .
ln -s $CDTOOL_PATH/hierarchy.json .
ln -s $CDTOOL_PATH/gen_config.py .
ln -s $CDTOOL_PATH/gen_config.yaml .
ln -s $CDTOOL_PATH/launch.sh .
cp $CDTOOL_PATH/run.sbatch .
ln -s ../lulesh_fgcd .
ln -s ../synthesize

