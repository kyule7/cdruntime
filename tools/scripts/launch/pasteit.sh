CD_EXEC=./synthesize
#### Set up Sweep Params ###############
export MPICH_ASYNC_PROGRESS=1
export I_MPICH_ASYNC_PROGRESS=1
export CD_EXEC_ITERS=0
export MPI_THREAD_MULTIPLE=1
export KEEP_TOTAL_FAILURE_RATE_SAME=0
export CD_EXEC_NAME=synthetic
export CD_EXEC_DETAILS=init
#export IBRUN_TASKS_PER_NODE=48
#########################################
MAX_RUNS=10
NUM_MEASURE=$(seq 1 $MAX_RUNS)
#########################################
INPUT_LIST=( "baseline" "commHeavy" "hetero" "hierarchy" "prsvHeavy" )


for COUNT in ${NUM_MEASURE}
do
  for INPUT in "${INPUT_LIST[@]}"
  do

if [ "$INPUT" == "prsvHeavy" ]; then
  export CD_DATA_GROW_UNIT=268435456
else
  export CD_DATA_GROW_UNIT=1048576
fi

start_time=$SECONDS

export CD_EXEC_DETAILS=$INPUT
#### Original ###############
unset CD_NO_PRESERVE
unset CD_NO_OPERATE
unset CD_NO_ERROR
export KEEP_TOTAL_FAILURE_RATE_SAME=0
export CD_NO_OPERATE=1
echo "ibrun $CD_EXEC ./${CD_EXEC_DETAILS}.json"
ibrun $CD_EXEC ./${CD_EXEC_DETAILS}.json

end_time=$SECONDS
duration=$(( end_time - start_time ))
echo "Iter:$COUNT, time:$duration"

#### No Preservation ###############
unset CD_NO_PRESERVE
unset CD_NO_OPERATE
unset CD_NO_ERROR
export KEEP_TOTAL_FAILURE_RATE_SAME=0
export CD_NO_PRESERVE=1
echo "ibrun $CD_EXEC ./${CD_EXEC_DETAILS}.json"
ibrun $CD_EXEC ./${CD_EXEC_DETAILS}.json

start_time=$SECONDS
duration=$(( start_time - end_time ))
echo "Iter:$COUNT, time:$duration"

#### Error Free ###############
unset CD_NO_PRESERVE
unset CD_NO_OPERATE
unset CD_NO_ERROR
export KEEP_TOTAL_FAILURE_RATE_SAME=0
export CD_NO_ERROR=1
echo "ibrun $CD_EXEC ./${CD_EXEC_DETAILS}.json"
ibrun $CD_EXEC ./${CD_EXEC_DETAILS}.json

end_time=$SECONDS
duration=$(( end_time - start_time ))
echo "Iter:$COUNT, time:$duration"

#### Error Injection ###############
unset CD_NO_PRESERVE
unset CD_NO_OPERATE
unset CD_NO_ERROR
export KEEP_TOTAL_FAILURE_RATE_SAME=1
echo "ibrun $CD_EXEC ./${CD_EXEC_DETAILS}.json"
ibrun $CD_EXEC ./${CD_EXEC_DETAILS}.json

start_time=$SECONDS
duration=$(( start_time - end_time ))
echo "Iter:$COUNT, time:$duration"

#### Error Injection ###############
unset CD_NO_PRESERVE
unset CD_NO_OPERATE
unset CD_NO_ERROR
export KEEP_TOTAL_FAILURE_RATE_SAME=64
echo "ibrun $CD_EXEC ./${CD_EXEC_DETAILS}.json"
ibrun $CD_EXEC ./${CD_EXEC_DETAILS}.json

end_time=$SECONDS
duration=$(( end_time - start_time ))
echo "Iter:$COUNT, time:$duration"
##############################################
  done

done

