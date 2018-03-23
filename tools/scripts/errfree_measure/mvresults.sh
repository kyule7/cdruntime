#!/bin/bash
RESULTDIR=results_0320
mkdir $RESULTDIR
cd run_8
mkdir $RESULTDIR
cp estimation.lulesh_* ./$RESULTDIR 
mv estimation.lulesh_* ../$RESULTDIR 
cp time_trace.lulesh_* ./$RESULTDIR 
mv time_trace.lulesh_* ../$RESULTDIR 
cp prof_trace.lulesh_* ./$RESULTDIR 
mv prof_trace.lulesh_* ../$RESULTDIR
cd ..
cd run_64
mkdir $RESULTDIR
cp estimation.lulesh_* ./$RESULTDIR 
mv estimation.lulesh_* ../$RESULTDIR 
cp time_trace.lulesh_* ./$RESULTDIR 
mv time_trace.lulesh_* ../$RESULTDIR 
cp prof_trace.lulesh_* ./$RESULTDIR 
mv prof_trace.lulesh_* ../$RESULTDIR
cd ..
cd run_216
mkdir $RESULTDIR
cp estimation.lulesh_* ./$RESULTDIR 
mv estimation.lulesh_* ../$RESULTDIR 
cp time_trace.lulesh_* ./$RESULTDIR 
mv time_trace.lulesh_* ../$RESULTDIR 
cp prof_trace.lulesh_* ./$RESULTDIR 
mv prof_trace.lulesh_* ../$RESULTDIR
cd ..
cd run_512
mkdir $RESULTDIR
cp estimation.lulesh_* ./$RESULTDIR 
mv estimation.lulesh_* ../$RESULTDIR 
cp time_trace.lulesh_* ./$RESULTDIR 
mv time_trace.lulesh_* ../$RESULTDIR 
cp prof_trace.lulesh_* ./$RESULTDIR 
mv prof_trace.lulesh_* ../$RESULTDIR
cd ..
cd run_1000
mkdir $RESULTDIR
cp estimation.lulesh_* ./$RESULTDIR 
mv estimation.lulesh_* ../$RESULTDIR 
cp time_trace.lulesh_* ./$RESULTDIR 
mv time_trace.lulesh_* ../$RESULTDIR 
cp prof_trace.lulesh_* ./$RESULTDIR 
mv prof_trace.lulesh_* ../$RESULTDIR
cd ..
