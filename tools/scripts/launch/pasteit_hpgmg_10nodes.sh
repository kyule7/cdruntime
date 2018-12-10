cd $SCRATCH/build_cd_gsrb

CD_EXEC=./bin/hpgmg-fv-hscd
CD_EXEC_HMCD=./bin/hpgmg-fv-hmcd
ls $CD_EXEC $CD_EXEC_HMCD

#### ibrun control env ###############
export IBRUN_TASKS_PER_NODE=24

#### Set up Sweep Params ###############
export MPICH_ASYNC_PROGRESS=1
export I_MPICH_ASYNC_PROGRESS=1
export CD_EXEC_ITERS=0
export KEEP_TOTAL_FAILURE_RATE_SAME=0
export CD_EXEC_NAME=hpgmg-fv-10nodes
#########################################
MAX_RUNS=5
NUM_MEASURE=$(seq 1 $MAX_RUNS)
#########################################
NUM_BOXES=8
BDIM_RANGE="`echo 6`"
TASK_RANGE="`echo 48 96 144 240`"


for COUNT in $NUM_MEASURE;
do
  for BOX_DIM in $BDIM_RANGE;
  do
    for NUM_TASKS in $TASK_RANGE;
    do
      start_time=$SECONDS
      
      echo "=========================================="
      
      #### Original ###############
      unset CD_NO_PRESERVE
      unset CD_NO_OPERATE
      unset CD_NO_ERROR
      export KEEP_TOTAL_FAILURE_RATE_SAME=0
      export CD_NO_OPERATE=1
      export CD_EXEC_DETAILS=${CD_EXEC_NAME}.$BOX_DIM.$NUM_BOXES.hscd
      . ~/checkenv_song.sh
      echo "ibrun -n $NUM_TASKS $CD_EXEC $BOX_DIM $NUM_BOXES &> log.noop.${CD_EXEC_DETAILS}.$NUM_TASKS.$COUNT.txt"
      ibrun -n $NUM_TASKS $CD_EXEC $BOX_DIM $NUM_BOXES &> log.noop.${CD_EXEC_DETAILS}.$NUM_TASKS.$COUNT.txt
      
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
      export CD_EXEC_DETAILS=${CD_EXEC_NAME}.$BOX_DIM.$NUM_BOXES.hscd
      . ~/checkenv_song.sh
      echo "ibrun -n $NUM_TASKS $CD_EXEC $BOX_DIM $NUM_BOXES &> log.noprv.${CD_EXEC_DETAILS}.$NUM_TASKS.$COUNT.txt"
      ibrun -n $NUM_TASKS $CD_EXEC $BOX_DIM $NUM_BOXES &> log.noprv.${CD_EXEC_DETAILS}.$NUM_TASKS.$COUNT.txt
      
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
      export CD_EXEC_DETAILS=${CD_EXEC_NAME}.$BOX_DIM.$NUM_BOXES.hscd
      . ~/checkenv_song.sh
      echo "ibrun -n $NUM_TASKS $CD_EXEC $BOX_DIM $NUM_BOXES &> log.noerr.hscd.${CD_EXEC_DETAILS}.$NUM_TASKS.$COUNT.txt"
      ibrun -n $NUM_TASKS $CD_EXEC $BOX_DIM $NUM_BOXES &> log.noerr.hscd.${CD_EXEC_DETAILS}.$NUM_TASKS.$COUNT.txt
      
      end_time=$SECONDS
      duration=$(( end_time - start_time ))
      echo "Iter:$COUNT, time:$duration"
      
      #### Error Free HMCD ###############
      unset CD_NO_PRESERVE
      unset CD_NO_OPERATE
      unset CD_NO_ERROR
      export KEEP_TOTAL_FAILURE_RATE_SAME=0
      export CD_NO_ERROR=1
      export CD_EXEC_DETAILS=${CD_EXEC_NAME}.$BOX_DIM.$NUM_BOXES.hmcd
      . ~/checkenv_song.sh
      echo "ibrun -n $NUM_TASKS $CD_EXEC_HMCD $BOX_DIM $NUM_BOXES &> log.noerr.hmcd.${CD_EXEC_DETAILS}.$NUM_TASKS.$COUNT.txt"
      ibrun -n $NUM_TASKS $CD_EXEC_HMCD $BOX_DIM $NUM_BOXES &> log.noerr.hmcd.${CD_EXEC_DETAILS}.$NUM_TASKS.$COUNT.txt
      
      end_time_2=$SECONDS
      duration=$(( end_time_2 - end_time ))
      echo "Iter:$COUNT, time:$duration"
      end_time=$end_time_2
      
      echo "=========================================="
      
      ##### Error Injection ###############
      #unset CD_NO_PRESERVE
      #unset CD_NO_OPERATE
      #unset CD_NO_ERROR
      #export KEEP_TOTAL_FAILURE_RATE_SAME=1
      #export CD_EXEC_DETAILS=${CD_EXEC_NAME}.$BOX_DIM.$NUM_BOXES.hscd
      #. ~/checkenv_song.sh
      #echo "ibrun -n $NUM_TASKS $CD_EXEC $BOX_DIM $NUM_BOXES &> log.err1.${CD_EXEC_DETAILS}.$NUM_TASKS.$COUNT.txt"
      #ibrun -n $NUM_TASKS $CD_EXEC $BOX_DIM $NUM_BOXES &> log.err1.${CD_EXEC_DETAILS}.$NUM_TASKS.$COUNT.txt
      #
      #start_time=$SECONDS
      #duration=$(( start_time - end_time ))
      #echo "Iter:$COUNT, time:$duration"
      #
      #echo "=========================================="
      #
      ##### Error Injection ###############
      #unset CD_NO_PRESERVE
      #unset CD_NO_OPERATE
      #unset CD_NO_ERROR
      #export KEEP_TOTAL_FAILURE_RATE_SAME=64
      #export CD_EXEC_DETAILS=${CD_EXEC_NAME}.$BOX_DIM.$NUM_BOXES.hscd
      #. ~/checkenv_song.sh
      #echo "ibrun -n $NUM_TASKS $CD_EXEC $BOX_DIM $NUM_BOXES &> log.err64.${CD_EXEC_DETAILS}.$NUM_TASKS.$COUNT.txt"
      #ibrun -n $NUM_TASKS $CD_EXEC $BOX_DIM $NUM_BOXES &> log.err64.${CD_EXEC_DETAILS}.$NUM_TASKS.$COUNT.txt
      #
      #end_time=$SECONDS
      #duration=$(( end_time - start_time ))
      #echo "Iter:$COUNT, time:$duration"
      #
      #echo "=========================================="
      
      ##############################################
    done
  done

done
