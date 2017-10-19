#!/bin/bash
TASK_LIST=(8 64 512)
for tasknum in "${TASK_LIST[@]}"
do
  for ((i=20;i<=100;i+=20))
  do
    ./run.sh $tasknum $i
  done

done
