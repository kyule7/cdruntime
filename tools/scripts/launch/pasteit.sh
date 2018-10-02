CD_EXEC=./synthesize

#### Set up Sweep Params ###############
export MPICH_ASYNC_PROGRESS=1
export I_MPICH_ASYNC_PROGRESS=1
export CD_EXEC_ITERS=0
export KEEP_TOTAL_FAILURE_RATE_SAME=0
#########################################
MAX_RUNS=1
NUM_MEASURE=$(seq 1 $MAX_RUNS)
#########################################

for COUNT in ${NUM_MEASURE}
do
start_time=$SECONDS

export CD_EXEC_NAME=synthetic
export CD_EXEC_DETAILS=prsvHeavy
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
echo "Iter:$iterations, time:$duration"

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
echo "Iter:$iterations, time:$duration"

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
echo "Iter:$iterations, time:$duration"

#### Error Injection ###############
unset CD_NO_PRESERVE
unset CD_NO_OPERATE
unset CD_NO_ERROR
export KEEP_TOTAL_FAILURE_RATE_SAME=64
echo "ibrun $CD_EXEC ./${CD_EXEC_DETAILS}.json"
ibrun $CD_EXEC ./${CD_EXEC_DETAILS}.json

start_time=$SECONDS
duration=$(( start_time - end_time ))
echo "Iter:$iterations, time:$duration"
##############################################


export CD_EXEC_DETAILS=hierarchy
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
echo "Iter:$iterations, time:$duration"

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
echo "Iter:$iterations, time:$duration"

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
echo "Iter:$iterations, time:$duration"

#### Error Injection ###############
unset CD_NO_PRESERVE
unset CD_NO_OPERATE
unset CD_NO_ERROR
export KEEP_TOTAL_FAILURE_RATE_SAME=64
echo "ibrun $CD_EXEC ./${CD_EXEC_DETAILS}.json"
ibrun $CD_EXEC ./${CD_EXEC_DETAILS}.json

start_time=$SECONDS
duration=$(( start_time - end_time ))
echo "Iter:$iterations, time:$duration"
##############################################
done
