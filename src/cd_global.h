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

#ifndef _CD_GLOBAL_H
#define _CD_GLOBAL_H
#include <stdint.h>
#include <assert.h>
#include <iostream>
#include <vector>
#include <map>


// This could be different from MPI program to PGAS program
// key is the unique ID from 0 for each CD node.
// value is the unique ID for mpi communication group or thread group.
// For MPI, there is a communicator number so it is the group ID,
// For PGAS, there should be a group ID, or we should generate it. 
#if _MPI_VER 
#include <mpi.h>

#define ROOT_COLOR MPI_COMM_WORLD
#define ROOT_HEAD_ID 0
#define INITIAL_COLOR MPI_COMM_NULL
typedef MPI_Comm ColorT;

#else

#define ROOT_COLOR 0 
#define ROOT_HEAD_ID 0
#define INITIAL_COLOR 0
typedef int ColorT;

#endif

#define INVALID_TASK_ID -1
#define INVALID_HEAD_ID -1

namespace cd {
  class CD;
  class HeadCD;
  class CDHandle;
  class CDPath;
  class CDEntry;
  class DataHandle;
  class Serializable;
  class NodeID;
  class CDID;
  class Packer; 
  class Unpacker; 
//  class Util;
  class CDEvent;
  class RegenObject;  
  class RecoverObject;
  class SysErrT;
#ifdef szhang
  //SZ
  class CommLog;
#endif

  enum CDErrT       { kOK=0, 
                      kAlreadyInit, 
                      kError,
                      kExecutionModeError,
                      kOutOfMemory,
                      kFileOpenError
                    };

//  enum SysErrT      { kWhat=1 };

  enum SysErrLoc    { kIntraCore=1, 
                      kCore=2, 
                      kProc=4, 
                      kNode=8, 
                      kModule=16, 
                      kCabinet=32, 
                      kCabinetGroup=64, 
                      kSystem=128 
                    };

  enum SysErrName   { kSoftMem=1, 
                      kDegradedMem=2, 
                      kSoftComm=4, 
                      kDegradedComm=8, 
                      kSoftComp=16, 
                      kDegradedResource=32, 
                      kHardResource=64, 
                      kFileSys=128 
                    };

  enum CDPGASUsageT { kShared=1, 
                      KPrivate 
                    };

  enum CDPreserveT  { kCopy=0, 
                      kRef, 
                      kRegen 
                    };

  enum CDType       { kStrict=0, 
                      kRelaxed };

  enum PreserveUseT { kUnsure=0, 
                      kReadOnly=1, 
                      kReadWrite=2 };

  /// Profile-related enumerator
  enum ProfileType      { LOOP_COUNT, 
                          EXEC_CYCLE, 
                          PRV_COPY_DATA, 
                          PRV_REF_DATA, 
                          OVERLAPPED_DATA, 
                          SYSTEM_BIT_VECTOR, 
                          MAX_PROFILE_DATA };

  enum ProfileFormat    { PRV, 
                          REC, 
                          BODY, 
                          MAX_FORMAT };

  enum PrvMediumT { kMemory=0,
                    kHDD,
                    kSSD,
                    kPFS};

#ifdef szhang
  //SZ
  enum CommLogErrT {kCommLogOK=0, 
                    kCommLogInitFailed, 
                    kCommLogAllocFailed,
                    kCommLogChildLogAllocFailed, 
                    kCommLogReInitFailed,
                    kCommLogCommLogModeFlip,
                    kCommLogError};
  
  //SZ
  enum CommLogMode { kGenerateLog=0,
                       kReplayLog
                    };
#endif


  class CDNameT;

  // Local CDHandle object and CD object are managed by CDPath (Local means the current process)

//  extern int myTaskID;

  extern CDHandle* CD_Init(int numproc=1, int myrank=0);
  extern void CD_Finalize();
//  extern uint64_t Util::gen_object_id_=0;

}

#define INITIAL_ERR_VAL kOK
#define DATA_MALLOC malloc
#define DATA_FREE free

//SZ: change the following macro to 1) add "Error: " before all error messages,
//    2) accept multiple arguments, and 3) assert(0) in ERROR_MESSAGE
//#define ERROR_MESSAGE(X) printf(X);
#define ERROR_MESSAGE(...) {printf("\nError: ");printf(__VA_ARGS__);assert(0);}
//#ifdef DEBUG
//#define DEBUG_PRINT(...) {printf(__VA_ARGS__);}
//#else
//#define DEBUG_PRINT(...) {}
//#endif



#if _DEBUG

//SZ: change the following macro to 
//#define PRINT_DEBUG(X) printf(X);
#define PRINT_DEBUG(...) {printf(__VA_ARGS__);}
#define PRINT_DEBUG2(X,Y) printf(X,Y);

#else

//SZ
//#define PRINT_DEBUG(X) printf(X);
#define PRINT_DEBUG(...) {}
#define PRINT_DEBUG2(X,Y) printf(X,Y);

#endif

#define MAX_FILE_PATH 2048

/* 
ISSUE 1 (Kyushick)
If we do if-else statement here and make a scope { } for that, does it make its own local scope in the stack?

void Foo(void)
{
  double X, Y, Z;
  ...
  if (IsTrue == true) { 
    int A, B, C, D;
    ...
    ...
    A = B + C; B = D * A;
    ...
  } 
  else {
    int E, F;
    ...
    E = F * E;
    ...
  }
  ...
}

there will be Foo's local scope in the stack and what about A, B, C, D, E, F ??
A, B, C, D, E, F's life cycle will be up to the end of if-then statement. 
So, I guess the stack will grow/shrink a bit due to this if-then statement.
So, if we setjmp or get context within this if-then statement's scope,
I think there will be some problem...

ISSUE 2 (Kyushick)
We are increasing the number of reexecution inside Begin(). So, the point of time when we mark rollback point is not after Begin() but before Begin()
*/

//#define CD_Begin(X) (X)->Begin(); if((X)->ctxt_prv_mode() ==CD::kExcludeStack) setjmp((X)->jump_buffer_);  else getcontext(&(X)->ctxt_) ; (X)->CommitPreserveBuff()

// Macros for setjump / getcontext
// So users should call this in their application, not call cd_handle->Begin().
#define CD_Begin(X) if((X)->ctxt_prv_mode() ==CD::kExcludeStack) setjmp((X)->jump_buffer_);  else getcontext(&(X)->ctxt_) ; (X)->CommitPreserveBuff(); (X)->Begin();
#define CD_Complete(X) (X)->Complete()   




#endif
