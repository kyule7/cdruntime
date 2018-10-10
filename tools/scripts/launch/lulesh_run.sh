CD_EXEC=./lulesh_fgcd
#### Set up Sweep Params ###############
export MPICH_ASYNC_PROGRESS=1
export I_MPICH_ASYNC_PROGRESS=1
export KEEP_TOTAL_FAILURE_RATE_SAME=0
export CD_EXEC_NAME=lulesh_fgcd
export CD_EXEC_ITERS=600
export CD_EXEC_DETAILS=40
#########################################
MAX_RUNS=10
NUM_MEASURE=$(seq 1 $MAX_RUNS)
#########################################
INPUT_LIST=( 40 60 80 )


for COUNT in ${NUM_MEASURE}
do
  for INPUT in "${INPUT_LIST[@]}"
  do
start_time=$SECONDS

export CD_EXEC_DETAILS=$INPUT
#### Original ###############
unset CD_NO_PRESERVE
unset CD_NO_OPERATE
unset CD_NO_ERROR
export KEEP_TOTAL_FAILURE_RATE_SAME=0
export CD_NO_OPERATE=1
echo "ibrun $CD_EXEC -i ${CD_EXEC_ITERS} -s ${CD_EXEC_DETAILS}"
ibrun $CD_EXEC -i ${CD_EXEC_ITERS} -s ${CD_EXEC_DETAILS}

end_time=$SECONDS
duration=$(( end_time - start_time ))
echo "Iter:$COUNT, time:$duration"

#### No Preservation ###############
unset CD_NO_PRESERVE
unset CD_NO_OPERATE
unset CD_NO_ERROR
export KEEP_TOTAL_FAILURE_RATE_SAME=0
export CD_NO_PRESERVE=1
echo "ibrun $CD_EXEC -i ${CD_EXEC_ITERS} -s ${CD_EXEC_DETAILS}"
ibrun $CD_EXEC -i ${CD_EXEC_ITERS} -s ${CD_EXEC_DETAILS}

start_time=$SECONDS
duration=$(( start_time - end_time ))
echo "Iter:$COUNT, time:$duration"

#### Error Free ###############
unset CD_NO_PRESERVE
unset CD_NO_OPERATE
unset CD_NO_ERROR
export KEEP_TOTAL_FAILURE_RATE_SAME=0
export CD_NO_ERROR=1
echo "ibrun $CD_EXEC -i ${CD_EXEC_ITERS} -s ${CD_EXEC_DETAILS}"
ibrun $CD_EXEC -i ${CD_EXEC_ITERS} -s ${CD_EXEC_DETAILS}

end_time=$SECONDS
duration=$(( end_time - start_time ))
echo "Iter:$COUNT, time:$duration"

#### Error Injection ###############
unset CD_NO_PRESERVE
unset CD_NO_OPERATE
unset CD_NO_ERROR
export KEEP_TOTAL_FAILURE_RATE_SAME=1
echo "ibrun $CD_EXEC -i ${CD_EXEC_ITERS} -s ${CD_EXEC_DETAILS}"
ibrun $CD_EXEC -i ${CD_EXEC_ITERS} -s ${CD_EXEC_DETAILS}

start_time=$SECONDS
duration=$(( start_time - end_time ))
echo "Iter:$COUNT, time:$duration"

#### Error Injection ###############
unset CD_NO_PRESERVE
unset CD_NO_OPERATE
unset CD_NO_ERROR
export KEEP_TOTAL_FAILURE_RATE_SAME=64
echo "ibrun $CD_EXEC -i ${CD_EXEC_ITERS} -s ${CD_EXEC_DETAILS}"
ibrun $CD_EXEC -i ${CD_EXEC_ITERS} -s ${CD_EXEC_DETAILS}

end_time=$SECONDS
duration=$(( end_time - start_time ))
echo "Iter:$COUNT, time:$duration"
##############################################
  done

done
