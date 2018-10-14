cd $SCRATCH/build_cd_gsrb

CD_EXEC=./bin/hpgmg-fv
ls $CD_EXEC

#### ibrun control env ###############
export IBRUN_TASKS_PER_NODE=24 

#### Set up Sweep Params ###############
export MPICH_ASYNC_PROGRESS=1
export I_MPICH_ASYNC_PROGRESS=1
export CD_EXEC_ITERS=0
export KEEP_TOTAL_FAILURE_RATE_SAME=0
export CD_EXEC_NAME=hpgmg-fv-9nodes
#########################################
MAX_RUNS=3
NUM_MEASURE=$(seq 1 $MAX_RUNS)
#########################################
NUM_BOXES=8
BDIM_RANGE="`echo {6..7}`"
TASK_RANGE="`echo 48 96 192 288 384`"


for COUNT in $NUM_MEASURE;
do
  for BOX_DIM in $BDIM_RANGE;
  do
    for NUM_TASKS in $TASK_RANGE;
    do
      start_time=$SECONDS
      
      export CD_EXEC_DETAILS=$BOX_DIM.$NUM_BOXES
      #### Original ###############
      unset CD_NO_PRESERVE
      unset CD_NO_OPERATE
      unset CD_NO_ERROR
      export KEEP_TOTAL_FAILURE_RATE_SAME=0
      export CD_NO_OPERATE=1
      . ~/checkenv_song.sh
      echo "ibrun -n $NUM_TASKS $CD_EXEC $BOX_DIM $NUM_BOXES &> log.noop.${CD_EXEC_NAME}.$NUM_TASKS.$BOX_DIM.$NUM_BOXES.$COUNT.txt"
      ibrun -n $NUM_TASKS $CD_EXEC $BOX_DIM $NUM_BOXES &> log.noop.${CD_EXEC_NAME}.$NUM_TASKS.$BOX_DIM.$NUM_BOXES.$COUNT.txt
      
      end_time=$SECONDS
      duration=$(( end_time - start_time ))
      echo "Iter:$COUNT, time:$duration"

      echo "=========================================="
      
      #### No Preservation ###############
      unset CD_NO_PRESERVE
      unset CD_NO_OPERATE
      unset CD_NO_ERROR
      export KEEP_TOTAL_FAILURE_RATE_SAME=0
      export CD_NO_PRESERVE=1
      . ~/checkenv_song.sh
      echo "ibrun -n $NUM_TASKS $CD_EXEC $BOX_DIM $NUM_BOXES &> log.noprv.${CD_EXEC_NAME}.$NUM_TASKS.$BOX_DIM.$NUM_BOXES.$COUNT.txt"
      ibrun -n $NUM_TASKS $CD_EXEC $BOX_DIM $NUM_BOXES &> log.noprv.${CD_EXEC_NAME}.$NUM_TASKS.$BOX_DIM.$NUM_BOXES.$COUNT.txt
      
      start_time=$SECONDS
      duration=$(( start_time - end_time ))
      echo "Iter:$COUNT, time:$duration"
      
      echo "=========================================="
      
      #### Error Free ###############
      unset CD_NO_PRESERVE
      unset CD_NO_OPERATE
      unset CD_NO_ERROR
      export KEEP_TOTAL_FAILURE_RATE_SAME=0
      export CD_NO_ERROR=1
      . ~/checkenv_song.sh
      echo "ibrun -n $NUM_TASKS $CD_EXEC $BOX_DIM $NUM_BOXES &> log.noerr.${CD_EXEC_NAME}.$NUM_TASKS.$BOX_DIM.$NUM_BOXES.$COUNT.txt"
      ibrun -n $NUM_TASKS $CD_EXEC $BOX_DIM $NUM_BOXES &> log.noerr.${CD_EXEC_NAME}.$NUM_TASKS.$BOX_DIM.$NUM_BOXES.$COUNT.txt
      
      end_time=$SECONDS
      duration=$(( end_time - start_time ))
      echo "Iter:$COUNT, time:$duration"
      
      echo "=========================================="
      
      #### Error Injection ###############
      unset CD_NO_PRESERVE
      unset CD_NO_OPERATE
      unset CD_NO_ERROR
      export KEEP_TOTAL_FAILURE_RATE_SAME=1
      . ~/checkenv_song.sh
      echo "ibrun -n $NUM_TASKS $CD_EXEC $BOX_DIM $NUM_BOXES &> log.err1.${CD_EXEC_NAME}.$NUM_TASKS.$BOX_DIM.$NUM_BOXES.$COUNT.txt"
      ibrun -n $NUM_TASKS $CD_EXEC $BOX_DIM $NUM_BOXES &> log.err1.${CD_EXEC_NAME}.$NUM_TASKS.$BOX_DIM.$NUM_BOXES.$COUNT.txt
      
      start_time=$SECONDS
      duration=$(( start_time - end_time ))
      echo "Iter:$COUNT, time:$duration"
      
      echo "=========================================="
      
      #### Error Injection ###############
      unset CD_NO_PRESERVE
      unset CD_NO_OPERATE
      unset CD_NO_ERROR
      export KEEP_TOTAL_FAILURE_RATE_SAME=64
      . ~/checkenv_song.sh
      echo "ibrun -n $NUM_TASKS $CD_EXEC $BOX_DIM $NUM_BOXES &> log.err64.${CD_EXEC_NAME}.$NUM_TASKS.$BOX_DIM.$NUM_BOXES.$COUNT.txt"
      ibrun -n $NUM_TASKS $CD_EXEC $BOX_DIM $NUM_BOXES &> log.err64.${CD_EXEC_NAME}.$NUM_TASKS.$BOX_DIM.$NUM_BOXES.$COUNT.txt
      
      end_time=$SECONDS
      duration=$(( end_time - start_time ))
      echo "Iter:$COUNT, time:$duration"
      
      echo "=========================================="
      
      ##############################################
    done
  done

done
