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

#ifdef comm_log

#include "cd_mpiwrapper.h"
#include "cd_path.h"
#include "cd_global.h"
#include "comm_log.h"

int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
  PRINT_DEBUG("here inside MPI_Send\n");
  PMPI_Send(buf, count, datatype, dest, tag, comm);

  return 0;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status *status)
{
  PRINT_DEBUG("here inside MPI_Recv\n");

  CDHandle * cur_cd = CDPath::GetCurrentCD();
  if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    PMPI_Recv(buf, count, datatype, src, tag, comm, status);

    PRINT_DEBUG("In kGenerateLog mode, generating new logs...\n");
    cur_cd->ptr_cd()->LogData(buf, count*sizeof(datatype));
  }
  else
  {
    PRINT_DEBUG("In kReplay mode, replaying from logs...\n");
    CommLogErrT ret = cur_cd->ptr_cd()->ReadData(buf, count*sizeof(datatype));
    if (ret == kCommLogCommLogModeFlip)
    {
      PRINT_DEBUG("Reached end of logs, and begin to generate logs...\n");
      PMPI_Recv(buf, count, datatype, src, tag, comm, status);
      cur_cd->ptr_cd()->LogData(buf, count*sizeof(datatype));
    }
  }

  return 0;
}


#endif
