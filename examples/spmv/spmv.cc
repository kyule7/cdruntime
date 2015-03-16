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

/*
 * @file spmv.cc
 * @author Mattan Erez
 * @date March 2014
 *
 * @brief Hierarchical SpMV CD Example
 *
 * The SpMV computation consists of iteratively multiplying a constant
 * matrix by an input vector.  The resultant vector is then used as the
 * input for the next iteration.  We assume that the matrix and vector
 * are block partitioned and assigned to multiple nodes and cores.  This
 * simple application demonstrates many of the features of CDs and how
 * they can be used to express efficient resilience.
 *
 * One of the advantages of containment domains is that preservation and
 * recovery can be tailored to exploit natural redundancy within the
 * machine.  A CD does not need to fully preserve its inputs at the
 * domain boundary; partial preservation may be utilized to increase
 * efficiency if an input naturally resides in multiple locations.
 * Examples for optimizing preserve/restore/recover routines include
 * restoring data from sibling CDs or other nodes which already have a
 * copy of the data for algorithmic reasons. 
 *
 * Hierarchical SpMV exhibits natural redundancy which can be
 * exploited through partial preservation and specialized
 * recovery. The input vector is distributed in such a way that
 * redundant copies of the vector are naturally distributed throughout
 * the machine. This is because there are \f$ N_0 \times N_0 \f$ fine-grained
 * sub-blocks of the matrix, but only \f$ N_0 \f$ sub-blocks in the vector.  
 *
 * This is a hierarchical/recursive form of SpMV that uses some
 * pseudocode just to demonstrate the usage of the CD API 
 */

#include "cd.h"

class VInRegen : public RegenType {
public:
  VInRegen(uint64_t task_num, uint subpartition) {
    task_num_ = task_num; subparition_ = subpartition;
  }

  CDErrType Regenerate(void* data_ptr, uint64_t len) {
    // During recovery (re-execution) Preserve acts like Restore
    // Writing this regeneration, which is really recovering data from
    // a sibling that has the same copy of the input vector is
    // difficult without assuming a specific parallelism
    // runtime. Unfortunately, both my MPI and UPC are rusty so I
    // can't do it right now. The idea is that we know which sibling
    // has data that we need based on the matrix partitioning and that
    // we know our subpartition number. We can then use the
    // parallelism runtime (or whoever tracked the task recursion) to
    // know which thread/rank/... this sibling is in, but we still
    // need to know its pointer to do a one-sided transfer of the data
    // because recovering this CD is independent of the sibling).
  }

protected:
  uint64_t task_num_;
  uint subpartition_;
};

/* @brief The recursive part that decomposes the problem for parallelism and containment.
 *
 * For simplicity, we assume that the input matrix has been
 * pre-partitioned to the appropriate number of levels to allow the
 * recursion to work correctly. 
 *
 */
void SpMVRecurse(const SparseMatrix* matrix, //!< [in] Partition of
					     //!< the matrix
		 const HierVector* v_in,     //!< [in] Partition of input vector
		 HierVector*       v_out,    //!< [in,out] Output
					     //!< vector for reduction
		 const CDHandle* current_cd, //!< [in] Handle to current CD
		 uint  num_tasks             //!< [in] Number of tasks
					     //!< at this level of recursion
		 		 ) {
  

  if (num_tasks > RECURSE_DEGREE) {
    uint tasks_per_child = num_tasks/RECURSE_DEGREE; // assume whole multiple
    for (int child=0; child < RECURSE_DEGREE; child++) {
      // assume that all iterations are all in parallel
      CDHandle* child_cd;
      // Creating the children CDs here so that we can more easily use
      // a collective mpi_comm_split-like Create method. This would be
      // easier if done internally by parallelism runtime/language
      child_cd = current_cd->CreateAndBegin(child, tasks_per_child);
      // Do some preservation
      CDEvent preserve_event;
      child_cd->Preserve(preserve_event,
			 matrix->Subpartition(child), // Pointer to
			 // start of subpartition within recursive matrix
			  matrix->SubpartitionLen(child), // Length in
							  // bytes of subpartition
			  kCopy|kParent, // Can either create another
					 // copy or use the parent's
					 // preserved matrix with
					 // appropriate offset
			  "Matrix",
			 "Matrix", matrix->PartitionOffset(),
			 0, 
			 kReadOnly
			 );
      // Regen object for input vector, assuming there is a
      // parallelism runtime that tracks recursion tree through task numbers
      VInRegen v_in_regen(ParRuntime::MyTaskNum(), child); 
      child_cd->Preserve(preserve_event, // Chain this event
			 v_in->Subpartition(child),
			 v_in->SubpartitionLen(child),
			 kCopy|kParent|kRegen,
			 "vIn",
			 "vIn", v_in->PartitionOffset(),
			 &v_in_regen
			 );
      // Do actual compute
      SpMVRecurse(matrix->Subpartition(child), 
		  v_in->Subpartition(child),
		  v_out->Subpartition(child),
		  child_cd, tasks_per_child);
      // Complete the CD
      preserve_event->Wait(); // Make sure preservation completed
      child_cd->Complete();
      child_cd->Destroy();
    }
  }
  else {
    for (int child=0; child < num_tasks; child++) {
      // assume that all iterations are all in parallel
      CDHandle* child_cd;
      CDHandle* child_cd = current_cd->Create();
      child_cd->Begin();
      // Do some preservation
      CDEvent preserve_event;
      child_cd->Preserve(preserve_event,
			 matrix->Partition(), // Pointer to
			 // start of subpartition within recursive matrix
			 matrix->PartitionLen(), // Length in
			 // bytes of subpartition
			 kCopy|kParent, // Can either create another
			 // copy or use the parent's
			 // preserved matrix with
			 // appropriate offset
			 "Matrix",
			 "Matrix", matrix->PartitionOffset(),
			 0,
			 kReadOnly
			 );
      VInRegen v_in_regen(ParRuntime::MyTaskNum(), child); 
      child_cd->Preserve(preserve_event, // Chain this event
			 v_in->Partition(),
			 v_in->SubpartitionLen(),
			 kCopy|kParent|kRegen,
			 "vIn",
			 "vIn", v_in->PartitionOffset(),
			 &v_in_regen
			 );
      // Do actual compute
      SpMVLeaf(matrix->Subpartition(child), 
	       v_in->Subpartition(child),
	       v_out->Subpartition(child),
	       child_cd, tasks_per_child);
      child_cd->Complete();
      child_cd->Destroy();
    }
  }
  
  v_out->ReduceSubpartitions(num_tasks);
}

/** @brief The leaf part that actually performs the computation
 *
 * For simplicity, we assume that the input matrix has been
 * pre-partitioned to the appropriate number of levels to allow the
 * recursion to work correctly. 
 *
 */
void SpMVLeaf(const SparseMatrix* matrix, //!< [in] Partition of
	      	                          //!< the matrix
	      const HierVector* v_in,     //!< [in] Partition of input vector
	      HierVector*       v_out,    //!< [in,out] Output
	                                  //!< vector for reduction
	      const CDHandle* current_cd, //!< [in] Handle to current CD
	      uint  num_tasks             //!< [in] Number of tasks
	                                  //!< at this level of recursion
	      ) {
  for (uint row=0; row < matrix->NumRows(); row++) {
    v_out[row] = 0.0;
    for (unit col = matrix->RowStart[row];
	 col <  matrix->RowStart[row+1];
	 col++) {
      uint prev_idx = 0;
      uint idx = matrix->Index[col];
      v_out[row] += matrix->NonZero[col]*v_in[idx];
      CDAssert(idx >= prev_idx); // data structure sanity check
      prev_idx = idx;
    }
  }
}
  
