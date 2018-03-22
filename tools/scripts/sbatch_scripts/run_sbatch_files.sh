#!/bin/bash
ceiled_div() {
    local num=$1
    local div=$2
    echo $(( (num + div - 1) / div ))
}

app=lulesh
cores_per_node=48
iterations=100
inputs=(40 60 80)
task_cnt=(8 64 216 512 100)

export NUM_MPI_RANKS=0
echo "inputs ${inputs[@]}"
echo "task_cnt $task_cnt"
for ntask in "${task_cnt[@]}"
do
  node_cnt=$(ceiled_div $ntask $cores_per_node)
  echo "sbatch --job-name=$app --nodes=$node_cnt --ntasks=$ntask --error=$app.%j.err --output=$app.%j.out --export=inputs=${inputs[@]},iterations=$iterations,NUM_MPI_RANKS=$ntask test.sbatch"
  sbatch --job-name=$app --nodes=$node_cnt --ntasks=$ntask --error=$app.%j.err --output=$app.%j.out --export=iterations=$iterations,NUM_MPI_RANKS=$ntask scripted_run.sbatch $iterations "${inputs[@]}" 
done

