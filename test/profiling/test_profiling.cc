/*
Copyright 2014, The University of Texas at Austin 
All rights reserved.

THIS FILE IS PART OF THE CONTAINMENT DOMAINS RUNTIME LIBRARY

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met: 

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer. 

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution. 

3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
//#include <sstream>
#include <string>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <mpi.h>
#include "cd.h"
#include "phase_tree.h"
using namespace cd;
using namespace std;
static int my_rank = 0;
static int comm_size = 1;
#define MYPRINT(...) \
  if(my_rank == 0) { \
    printf(__VA_ARGS__); \
  }

uint32_t BeginPhase(uint32_t level, const string &label) {
  uint32_t phase = 0;
  if(phaseTree.current_ != NULL) {
    phase = phaseTree.current_->GetPhaseNode(level, label);
  } else {
    if(phaseTree.root_ == NULL)
      phase = phaseTree.Init(level, label);
    else 
      phase = phaseTree.ReInit();
  }
  phaseTree.current_->profile_.RecordBegin(false, false);
  MYPRINT("[%s %d] phase: %u\n", __func__, my_rank, phase);
  usleep(10000);
  return phase;
}

void CompletePhase(uint32_t phase, bool err_free=true)
{
  phaseTree.current_->profile_.RecordComplete(false);
  assert(phaseTree.current_);
  assert(phaseTree.current_->phase_ == phase);
  phaseTree.current_->IncSeqID(err_free);
  MYPRINT("[%s %d] phase: %u\n", __func__, my_rank, phase);
  phaseTree.current_->last_completed_phase = phaseTree.current_->phase_;
  phaseTree.current_ = phaseTree.current_->parent_;
}

uint32_t level_gen = 0;
void TestPhase(void) 
{
  uint32_t level = level_gen++;
  uint32_t phase = BeginPhase(level, "Root");
  if(my_rank == 0) {
    phaseTree.current_->PrintNode(true, stdout);
  }
  {
    uint32_t level = level_gen++;
    uint32_t phase = 0;
    for(int i = 0; i<5; i++) {
      phase = BeginPhase(level, "Lv1_first");
      cd::begin_clk = CD_CLOCK();
      phaseTree.current_->profile_.RecordData("Lv1_first_prv1", 1000, kCopy, false);
      if(my_rank == 0) {
        phaseTree.current_->PrintNode(true, stdout);
      }
      CompletePhase(phase); // Lv1
    }
    for(int i = 0; i<10; i++) {
      phase = BeginPhase(level, "Lv1_second");
      cd::begin_clk = CD_CLOCK();
      phaseTree.current_->profile_.RecordData("Lv1_second_prv1", 1000, kCopy, false);
      if(my_rank == 0) {
        phaseTree.current_->PrintNode(true, stdout);
      }
      CompletePhase(phase); // Lv1
    }
  }

  CompletePhase(phase);
}

int main(int argc, char *argv[])
{
  MPI_Init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  
  printf("[%s %d] Begin Test\n\n", __func__, my_rank);
 
  MPI_Barrier(MPI_COMM_WORLD);
  TestPhase();

  MPI_Finalize();
  return 0;
}

