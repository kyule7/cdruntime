#!/bin/bash
ceiled_div() {
    local num=$1
    local div=$2
    echo $(( (num + div - 1) / div ))
}

app=lulesh_stmp2
cores_per_node=48
iterations=50
max_run=1
inputs=(40 60 80)
task_cnt=(64)
#inputs=(40 60 80)
#task_cnt=(8 64 216 512 100)

echo "inputs ${inputs[@]}"
echo "task_cnt $task_cnt"
for ntask in "${task_cnt[@]}"
do
  node_cnt=$(ceiled_div $ntask $cores_per_node)
  export NUM_MPI_RANKS=$ntask
  export MPICH_ASYNC_PROGRESS=1
  export OMP_NUM_THREADS=1
  echo "sbatch --job-name=$app --nodes=$node_cnt --ntasks=$ntask --error=$app.%j.err --output=$app.%j.out --export=iterations=$iterations,NUM_MPI_RANKS=$ntask scripted_run.sbatch $app $iterations ${inputs[*]}"
  echo "sbatch --job-name=$app --nodes=$node_cnt --ntasks=$ntask --error=$app.%j.err --output=$app.%j.out --export=NUM_MPI_RANKS=$ntask,MPICH_ASYNC_PROGRESS=1,OMP_NUM_THREADS=1,ALL scripted_run.sbatch app:./$app iter:$iterations maxrun:$max_run inputs:${inputs[*]}"
  sbatch --job-name=$app --nodes=$node_cnt --ntasks=$ntask --error=$app.%j.err --output=$app.%j.out --export=NUM_MPI_RANKS=$ntask,MPICH_ASYNC_PROGRESS=1,OMP_NUM_THREADS=1,ALL scripted_run.sbatch ./$app $iterations $max_run "${inputs[@]}" 
done

