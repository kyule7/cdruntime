//------------------------------------------------------------------------------------------------------------------------------
// Copyright Notice 
//------------------------------------------------------------------------------------------------------------------------------
// HPGMG, Copyright (c) 2014, The Regents of the University of
// California, through Lawrence Berkeley National Laboratory (subject to
// receipt of any required approvals from the U.S. Dept. of Energy).  All
// rights reserved.
// 
// If you have questions about your rights to use or distribute this
// software, please contact Berkeley Lab's Technology Transfer Department
// at  TTD@lbl.gov.
// 
// NOTICE.  This software is owned by the U.S. Department of Energy.  As
// such, the U.S. Government has been granted for itself and others
// acting on its behalf a paid-up, nonexclusive, irrevocable, worldwide
// license in the Software to reproduce, prepare derivative works, and
// perform publicly and display publicly.  Beginning five (5) years after
// the date permission to assert copyright is obtained from the U.S.
// Department of Energy, and subject to any subsequent five (5) year
// renewals, the U.S. Government is granted for itself and others acting
// on its behalf a paid-up, nonexclusive, irrevocable, worldwide license
// in the Software to reproduce, prepare derivative works, distribute
// copies to the public, perform publicly and display publicly, and to
// permit others to do so.
//------------------------------------------------------------------------------------------------------------------------------
// Samuel Williams
// SWWilliams@lbl.gov
// Lawrence Berkeley National Lab
//------------------------------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
//------------------------------------------------------------------------------------------------------------------------------
#ifdef USE_MPI
#include <mpi.h>
#endif
#ifdef _OPENMP
#include <omp.h>
#endif
//------------------------------------------------------------------------------------------------------------------------------
#include "timers.h"
#include "defines.h"
#include "level.h"
#include "mg.h"
#include "operators.h"
#include "solvers.h"
//------------------------------------------------------------------------------------------------------------------------------
#if CD
#include "cd.h"
#elif SCR
#include "scr.h"
#endif
//------------------------------------------------------------------------------------------------------------------------------
void bench_hpgmg(mg_type *all_grids, int onLevel, double a, double b, double dtol, double rtol){
  #if CD
  cd_handle_t * cd_bench = getleafcd();
  cd_begin(cd_bench, "bench");
  cd_preserve(cd_bench, all_grids, sizeof(mg_type), kCopy, "bench_all_grids", NULL);

  //SZ: FIXME: should verify following preservations can use kRef
  cd_preserve(cd_bench, &a, sizeof(a), kRef, "a", "a");
  cd_preserve(cd_bench, &b, sizeof(b), kRef, "b", "a");
  cd_preserve(cd_bench, &dtol, sizeof(dtol), kRef, "dtol", "dtol");
  cd_preserve(cd_bench, &rtol, sizeof(rtol), kRef, "rtol", "rtol");
  cd_preserve(cd_bench, &onLevel, sizeof(onLevel), kCopy, "onLevel", NULL);
  #endif
     int     doTiming;
     int    minSolves = 10; // do at least minSolves MGSolves
  double timePerSolve = 0;

  for(doTiming=0;doTiming<=1;doTiming++){ // first pass warms up, second pass times

    #ifdef USE_HPM // IBM performance counters for BGQ...
    if( (doTiming==1) && (onLevel==0) )HPM_Start("FMGSolve()");
    #endif

    #ifdef USE_MPI
    double minTime   = 60.0; // minimum time in seconds that the benchmark should run
    double startTime = MPI_Wtime();
    if(doTiming==1){
      if((minTime/timePerSolve)>minSolves)minSolves=(minTime/timePerSolve); // if one needs to do more than minSolves to run for minTime, change minSolves
    }
    #endif

    if(all_grids->levels[onLevel]->my_rank==0){
      if(doTiming==0){fprintf(stdout,"\n\n===== Warming up by running %d solves ==========================================\n",minSolves);}
                 else{fprintf(stdout,"\n\n===== Running %d solves ========================================================\n",minSolves);}
      fflush(stdout);
    }

    int numSolves =  0; // solves completed
    MGResetTimers(all_grids);
  #if CD
    int num_tasks;
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
    //cd_handle_t * cd_mgsolve = cd_create(cd_bench, num_tasks, "cd_mgsolve", kRelaxed | kDRAM, 0x0000FFFF);
    cd_handle_t * cd_mgsolve = cd_create(cd_bench, 1, "cd_mgsolve", kStrict | kDRAM, 0x0000FFFF);
  #endif
    while( (numSolves<minSolves) ){
      #if CD
      cd_begin(cd_mgsolve, "cd_mgsolve");
      #elif SCR
      int need_checkpoint;
      SCR_Need_checkpoint(&need_checkpoint);
      if (need_checkpoint){
        SCR_Start_checkpoint();
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);

        char checkpoint_file[256];
        sprintf(checkpoint_file, "scr_ckpt_files/rank_%d.ckpt", rank);

        char scr_file[SCR_MAX_FILENAME];
        SCR_Route_file(checkpoint_file, scr_file);
        //printf("%d: scr_file:%s\n", rank, scr_file);

        /*each process opens scr_file, takes checkpoints, and closes the file*/
        FILE *fs = fopen(scr_file, "w");
        int valid=0;
        if (fs != NULL){
          valid = 1;

          // take checkpoints
          size_t nwrites=0;
          nwrites+=fwrite(all_grids, sizeof(mg_type), 1, fs);
          nwrites+=fwrite(&a, sizeof(a), 1, fs);
          nwrites+=fwrite(&b, sizeof(b), 1, fs);
          nwrites+=fwrite(&dtol, sizeof(dtol), 1, fs);
          nwrites+=fwrite(&rtol, sizeof(rtol), 1, fs);
          nwrites+=fwrite(&onLevel, sizeof(onLevel), 1, fs);

          //if (nwrites != (sizeof(mg_type)+sizeof(a)+sizeof(b)+sizeof(dtol)+sizeof(rtol)+sizeof(onLevel)))
          if (nwrites != 6)
            valid = 0;

          if (fclose(fs)!=0) valid=0;
        }

        SCR_Complete_checkpoint(valid);
      }
      #endif

      zero_vector(all_grids->levels[onLevel],VECTOR_U);
      #ifdef USE_FCYCLES
      FMGSolve(all_grids,onLevel,VECTOR_U,VECTOR_F,a,b,dtol,rtol);
      #else
       MGSolve(all_grids,onLevel,VECTOR_U,VECTOR_F,a,b,dtol,rtol);
      #endif
      numSolves++;

      #if CD
      cd_complete(cd_mgsolve);
      #endif
    }
  #if CD
    cd_destroy(cd_mgsolve);
  #endif

    #ifdef USE_MPI
    if(doTiming==0){
      double endTime = MPI_Wtime();
      timePerSolve = (endTime-startTime)/numSolves;
      MPI_Bcast(&timePerSolve,1,MPI_DOUBLE,0,MPI_COMM_WORLD); // after warmup, process 0 broadcasts the average time per solve (consensus)
    }
    #endif

    #ifdef USE_HPM // IBM performance counters for BGQ...
    if( (doTiming==1) && (onLevel==0) )HPM_Stop("FMGSolve()");
    #endif
  }
  #if CD
  cd_complete(cd_bench);
  #endif
}


//------------------------------------------------------------------------------------------------------------------------------
int main(int argc, char **argv){
  int my_rank=0;
  int num_tasks=1;
  int OMP_Threads = 1;

  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
  #ifdef _OPENMP
  #pragma omp parallel 
  {
    #pragma omp master
    {
      OMP_Threads = omp_get_num_threads();
    }
  }
  #endif
    
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
  // initialize MPI and HPM
  #ifdef USE_MPI
  int    actual_threading_model = -1;
  int requested_threading_model = -1;
      requested_threading_model = MPI_THREAD_SINGLE;
    //requested_threading_model = MPI_THREAD_FUNNELED;
    //requested_threading_model = MPI_THREAD_SERIALIZED;
    //requested_threading_model = MPI_THREAD_MULTIPLE;
    #ifdef _OPENMP
      requested_threading_model = MPI_THREAD_FUNNELED;
    //requested_threading_model = MPI_THREAD_SERIALIZED;
    //requested_threading_model = MPI_THREAD_MULTIPLE;
    #endif
  MPI_Init_thread(&argc, &argv, requested_threading_model, &actual_threading_model);
  MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  #ifdef USE_HPM // IBM HPM counters for BGQ...
  HPM_Init();
  #endif
  #endif // USE_MPI


  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
  // parse the arguments...
  int     log2_box_dim           =  6; // 64^3
  int     target_boxes_per_rank  =  1;
//int64_t target_memory_per_rank = -1; // not specified
  int64_t box_dim                = -1;
  int64_t boxes_in_i             = -1;
  int64_t target_boxes           = -1;

  if(argc==3){
             log2_box_dim=atoi(argv[1]);
    target_boxes_per_rank=atoi(argv[2]);

    if(log2_box_dim>9){
      // NOTE, in order to use 32b int's for array indexing, box volumes must be less than 2^31 doubles
      if(my_rank==0){fprintf(stderr,"log2_box_dim must be less than 10\n");}
      #ifdef USE_MPI
      MPI_Finalize();
      #endif
      exit(0);
    }

    if(log2_box_dim<4){
      if(my_rank==0){fprintf(stderr,"log2_box_dim must be at least 4\n");}
      #ifdef USE_MPI
      MPI_Finalize();
      #endif
      exit(0);
    }

    if(target_boxes_per_rank<1){
      if(my_rank==0){fprintf(stderr,"target_boxes_per_rank must be at least 1\n");}
      #ifdef USE_MPI
      MPI_Finalize();
      #endif
      exit(0);
    }

    #ifndef MAX_COARSE_DIM
    #define MAX_COARSE_DIM 11
    #endif
    box_dim=1<<log2_box_dim;
    target_boxes = (int64_t)target_boxes_per_rank*(int64_t)num_tasks;
    boxes_in_i = -1;
    int64_t bi;
    for(bi=1;bi<1000;bi++){ // search all possible problem sizes to find acceptable boxes_in_i
      int64_t total_boxes = bi*bi*bi;
      if(total_boxes<=target_boxes){
        int64_t coarse_grid_dim = box_dim*bi;
        while( (coarse_grid_dim%2) == 0){coarse_grid_dim=coarse_grid_dim/2;}
        if(coarse_grid_dim<=MAX_COARSE_DIM){
          boxes_in_i = bi;
        }
      }
    }
    if(boxes_in_i<1){
      if(my_rank==0){fprintf(stderr,"failed to find an acceptable problem size\n");}
      #ifdef USE_MPI
      MPI_Finalize();
      #endif
      exit(0);
    }
  } // argc==3

  #if 0
  else if(argc==2){ // interpret argv[1] as target_memory_per_rank
    char *ptr = argv[1];
    char *tmp;
    target_memory_per_rank = strtol(ptr,&ptr,10);
    if(target_memory_per_rank<1){
      if(my_rank==0){fprintf(stderr,"unrecognized target_memory_per_rank... '%s'\n",argv[1]);}
      #ifdef USE_MPI
      MPI_Finalize();
      #endif
      exit(0);
    }
    tmp=strstr(ptr,"TB");if(tmp){ptr=tmp+2;target_memory_per_rank *= (uint64_t)(1<<30)*(1<<10);}
    tmp=strstr(ptr,"GB");if(tmp){ptr=tmp+2;target_memory_per_rank *= (uint64_t)(1<<30);}
    tmp=strstr(ptr,"MB");if(tmp){ptr=tmp+2;target_memory_per_rank *= (uint64_t)(1<<20);}
    tmp=strstr(ptr,"tb");if(tmp){ptr=tmp+2;target_memory_per_rank *= (uint64_t)(1<<30)*(1<<10);}
    tmp=strstr(ptr,"gb");if(tmp){ptr=tmp+2;target_memory_per_rank *= (uint64_t)(1<<30);}
    tmp=strstr(ptr,"mb");if(tmp){ptr=tmp+2;target_memory_per_rank *= (uint64_t)(1<<20);}
    if( (ptr) && (*ptr != '\0') ){
      if(my_rank==0){fprintf(stderr,"unrecognized units... '%s'\n",ptr);}
      #ifdef USE_MPI
      MPI_Finalize();
      #endif
      exit(0);
    }
    // FIX, now search for an 'acceptable' box_dim and boxes_in_i constrained by target_memory_per_rank, num_tasks, and MAX_COARSE_DIM
  } // argc==2
  #endif


  else{
    if(my_rank==0){fprintf(stderr,"usage: ./hpgmg-fv  [log2_box_dim]  [target_boxes_per_rank]\n");}
                 //fprintf(stderr,"       ./hpgmg-fv  [target_memory_per_rank[MB,GB,TB]]\n");}
    #ifdef USE_MPI
    MPI_Finalize();
    #endif
    exit(0);
  }



#if CD
  cd_handle_t* root_cd = cd_init(num_tasks, my_rank, kHDD);
  cd_begin(root_cd, "root");
#elif SCR
  SCR_Init();
#endif


  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  if(my_rank==0){
  fprintf(stdout,"\n\n");
  fprintf(stdout,"********************************************************************************\n");
  fprintf(stdout,"***                            HPGMG-FV Benchmark                            ***\n");
  fprintf(stdout,"********************************************************************************\n");
  #ifdef USE_MPI
       if(requested_threading_model == MPI_THREAD_MULTIPLE  )fprintf(stdout,"Requested MPI_THREAD_MULTIPLE, ");
  else if(requested_threading_model == MPI_THREAD_SINGLE    )fprintf(stdout,"Requested MPI_THREAD_SINGLE, ");
  else if(requested_threading_model == MPI_THREAD_FUNNELED  )fprintf(stdout,"Requested MPI_THREAD_FUNNELED, ");
  else if(requested_threading_model == MPI_THREAD_SERIALIZED)fprintf(stdout,"Requested MPI_THREAD_SERIALIZED, ");
  else if(requested_threading_model == MPI_THREAD_MULTIPLE  )fprintf(stdout,"Requested MPI_THREAD_MULTIPLE, ");
  else                                                       fprintf(stdout,"Requested Unknown MPI Threading Model (%d), ",requested_threading_model);
       if(actual_threading_model    == MPI_THREAD_MULTIPLE  )fprintf(stdout,"got MPI_THREAD_MULTIPLE\n");
  else if(actual_threading_model    == MPI_THREAD_SINGLE    )fprintf(stdout,"got MPI_THREAD_SINGLE\n");
  else if(actual_threading_model    == MPI_THREAD_FUNNELED  )fprintf(stdout,"got MPI_THREAD_FUNNELED\n");
  else if(actual_threading_model    == MPI_THREAD_SERIALIZED)fprintf(stdout,"got MPI_THREAD_SERIALIZED\n");
  else if(actual_threading_model    == MPI_THREAD_MULTIPLE  )fprintf(stdout,"got MPI_THREAD_MULTIPLE\n");
  else                                                       fprintf(stdout,"got Unknown MPI Threading Model (%d)\n",actual_threading_model);
  #endif
  fprintf(stdout,"%d MPI Tasks of %d threads\n",num_tasks,OMP_Threads);
  fprintf(stdout,"\n\n===== Benchmark setup ==========================================================\n");
  }

  #if CD
  cd_handle_t * cd_l1 = cd_create(getcurrentcd(), 1, "cd_l1", kStrict | kHDD, 0xFFFFFFFF);
  cd_begin(cd_l1, "cd_l1_mgbuild");
  #endif

  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // create the fine level...
  #ifdef USE_PERIODIC_BC
  int bc = BC_PERIODIC;
  int minCoarseDim = 2; // avoid problems with black box calculation of D^{-1} for poisson with periodic BC's on a 1^3 grid
  #else
  int bc = BC_DIRICHLET;
  int minCoarseDim = 1; // assumes you can drop order on the boundaries
  #endif
  level_type level_h;
  int ghosts=stencil_get_radius();
  
  //SZNOTE: two MPI_Allreduce to gather information...
  create_level(&level_h,boxes_in_i,box_dim,ghosts,VECTORS_RESERVED,bc,my_rank,num_tasks);
  #ifdef USE_HELMHOLTZ
  double a=1.0;double b=1.0; // Helmholtz
  if(my_rank==0)fprintf(stdout,"  Creating Helmholtz (a=%f, b=%f) test problem\n",a,b);
  #else
  double a=0.0;double b=1.0; // Poisson
  if(my_rank==0)fprintf(stdout,"  Creating Poisson (a=%f, b=%f) test problem\n",a,b);
  #endif
  double h=1.0/( (double)boxes_in_i*(double)box_dim );  // [0,1]^3 problem
  initialize_problem(&level_h,h,a,b);                   // initialize VECTOR_ALPHA, VECTOR_BETA*, and VECTOR_F
  
  //SZNOTE: restriction and exchange boundaries within, lots of MPI non-blocking communications
  rebuild_operator(&level_h,NULL,a,b);                  // calculate Dinv and lambda_max
  if(level_h.boundary_condition.type == BC_PERIODIC){   // remove any constants from the RHS for periodic problems
    double average_value_of_f = mean(&level_h,VECTOR_F);
    if(average_value_of_f!=0.0){
      if(my_rank==0){fprintf(stderr,"  WARNING... Periodic boundary conditions, but f does not sum to zero... mean(f)=%e\n",average_value_of_f);}
      shift_vector(&level_h,VECTOR_F,VECTOR_F,-average_value_of_f);
    }
  }


  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // create the MG hierarchy...
  mg_type MG_h;
  //SZNOTE: MPI_Comm_split inside and MPI_Allreduce on splitted communicators
  MGBuild(&MG_h,&level_h,a,b,minCoarseDim);             // build the Multigrid Hierarchy 

  #if CD
  cd_complete(cd_l1);
  #endif

  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  // HPGMG-500 benchmark proper
  // evaluate performance on problem sizes of h, 2h, and 4h
  // (i.e. examine dynamic range for problem sizes N, N/8, and N/64)
//double dtol=1e-15;double rtol=  0.0; // converged if ||D^{-1}(b-Ax)|| < dtol
  double dtol=  0.0;double rtol=1e-10; // converged if ||b-Ax|| / ||b|| < rtol
  int l;
  #ifndef TEST_ERROR
  #if CD
  cd_begin(cd_l1, "cd_l1_mgsolve");
  cd_preserve(cd_l1, &dtol, sizeof(dtol), kCopy, "dtol", NULL);
  cd_preserve(cd_l1, &rtol, sizeof(rtol), kCopy, "rtol", NULL);
  cd_preserve(cd_l1, &a, sizeof(a), kCopy, "a", NULL);
  cd_preserve(cd_l1, &b, sizeof(b), kCopy, "b", NULL);

  // SZ: FIXME: a relaxed CD here, but should explore communication pattern to change to strict...
  //cd_handle_t *cd_l2 = cd_create(cd_l1, num_tasks, "cd_l2", kRelaxed | kDRAM, 0x0000FFFF);
  cd_handle_t *cd_l2 = cd_create(cd_l1, 1, "cd_l2", kStrict | kDRAM, 0x0000FFFF);
  #endif

  double AverageSolveTime[3];
  for(l=0;l<3;l++){
    if(l>0)restriction(MG_h.levels[l],VECTOR_F,MG_h.levels[l-1],VECTOR_F,RESTRICT_CELL);
    //SZNOTE: actual solver, lots of MPI communications
    bench_hpgmg(&MG_h,l,a,b,dtol,rtol);
    AverageSolveTime[l] = (double)MG_h.timers.MGSolve / (double)MG_h.MGSolves_performed;
    if(my_rank==0){fprintf(stdout,"\n\n===== Timing Breakdown =========================================================\n");}
    MGPrintTiming(&MG_h,l);
  }

  if(my_rank==0){
    #ifdef CALIBRATE_TIMER
    double _timeStart=getTime();sleep(1);double _timeEnd=getTime();
    double SecondsPerCycle = (double)1.0/(double)(_timeEnd-_timeStart);
    #else
    double SecondsPerCycle = 1.0;
    #endif
    fprintf(stdout,"\n\n===== Performance Summary ======================================================\n");
    for(l=0;l<3;l++){
      double DOF = (double)MG_h.levels[l]->dim.i*(double)MG_h.levels[l]->dim.j*(double)MG_h.levels[l]->dim.k;
      double seconds = SecondsPerCycle*(double)AverageSolveTime[l];
      double DOFs = DOF / seconds;
      fprintf(stdout,"  h=%0.15e  DOF=%0.15e  time=%0.6f  DOF/s=%0.3e  MPI=%d  OMP=%d\n",MG_h.levels[l]->h,DOF,seconds,DOFs,num_tasks,OMP_Threads);
    }
  }

  #if CD
  cd_destroy(cd_l2);
  cd_complete(cd_l1);
  #endif
  #endif

  #if CD
  cd_destroy(cd_l1);
  #endif

  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  if(my_rank==0){fprintf(stdout,"\n\n===== Richardson error analysis ================================================\n");}
  // solve A^h u^h = f^h
  // solve A^2h u^2h = f^2h
  // solve A^4h u^4h = f^4h
  // error analysis...
  MGResetTimers(&MG_h);
  for(l=0;l<3;l++){
    if(l>0)restriction(MG_h.levels[l],VECTOR_F,MG_h.levels[l-1],VECTOR_F,RESTRICT_CELL);
           zero_vector(MG_h.levels[l],VECTOR_U);
    #ifdef USE_FCYCLES
    FMGSolve(&MG_h,l,VECTOR_U,VECTOR_F,a,b,dtol,rtol);
    #else
     MGSolve(&MG_h,l,VECTOR_U,VECTOR_F,a,b,dtol,rtol);
    #endif
  }
  richardson_error(&MG_h,0,VECTOR_U);


  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  if(my_rank==0){fprintf(stdout,"\n\n===== Deallocating memory ======================================================\n");}
  MGDestroy(&MG_h);
  destroy_level(&level_h);


  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  if(my_rank==0){fprintf(stdout,"\n\n===== Done =====================================================================\n");}

#if CD
  cd_complete(root_cd);
  cd_finalize();
#elif SCR
  SCR_Finalize();
#endif


  #ifdef USE_MPI
  #ifdef USE_HPM // IBM performance counters for BGQ...
  HPM_Print();
  #endif
  MPI_Finalize();
  #endif
  return(0);
  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
}
