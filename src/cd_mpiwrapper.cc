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

// ------------------------------------------------------------------------------------------------------------
// blocking p2p communication
// ------------------------------------------------------------------------------------------------------------

// blocking send: this one will return when the buffer is ready to reuse, but not guarantee messages
// have been received, because small messages may be copied into internal buffers
// this function is thread-safe
int MPI_Send(void *buf, 
             int count, 
             MPI_Datatype datatype, 
             int dest, 
             int tag, 
             MPI_Comm comm)
{
  int mpi_ret=0;
  PRINT_DEBUG("here inside MPI_Send\n");
  PRINT_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cd = CDPath::GetCurrentCD();
  if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    mpi_ret = PMPI_Send(buf, count, datatype, dest, tag, comm);

    PRINT_DEBUG("In kGenerateLog mode, generating new logs...\n");
    //cur_cd->ptr_cd()->LogData(&buf, sizeof(buf));
    cur_cd->ptr_cd()->LogData(buf, 0);
  }
  else
  {
    PRINT_DEBUG("In kReplay mode, replaying from logs...\n");
    CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(buf, 0);
    if (ret == kCommLogCommLogModeFlip)
    {
      PRINT_DEBUG("Reached end of logs, and begin to generate logs...\n");
      mpi_ret = PMPI_Send(buf, count, datatype, dest, tag, comm);
      //cur_cd->ptr_cd()->LogData(&buf, sizeof(buf));
      cur_cd->ptr_cd()->LogData(buf, 0);
    }
  }

  return mpi_ret;
}

// synchronous send: the send will complete until the corresponding receive has been posted
// note: this function is thread-safe
int MPI_Ssend(void *buf, 
              int count, 
              MPI_Datatype datatype, 
              int dest, 
              int tag, 
              MPI_Comm comm)
{
  int mpi_ret=0;
  PRINT_DEBUG("here inside MPI_Ssend\n");
  PRINT_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cd = CDPath::GetCurrentCD();
  if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    mpi_ret = PMPI_Ssend(buf, count, datatype, dest, tag, comm);

    PRINT_DEBUG("In kGenerateLog mode, generating new logs...\n");
    //cur_cd->ptr_cd()->LogData(&buf, sizeof(buf));
    cur_cd->ptr_cd()->LogData(buf, 0);
  }
  else
  {
    PRINT_DEBUG("In kReplay mode, replaying from logs...\n");
    //CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(&buf, sizeof(buf));
    CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(buf, 0);
    if (ret == kCommLogCommLogModeFlip)
    {
      PRINT_DEBUG("Reached end of logs, and begin to generate logs...\n");
      mpi_ret = PMPI_Ssend(buf, count, datatype, dest, tag, comm);
      //cur_cd->ptr_cd()->LogData(&buf, sizeof(buf));
      cur_cd->ptr_cd()->LogData(buf, 0);
    }
  }

  return mpi_ret;
}

// ready send: this function can only be called when user is sure receive call has been posted
// this function may save some handshakes for communication in some MPI implementations
// note: this function is thread-safe
int MPI_Rsend(void *buf, 
              int count, 
              MPI_Datatype datatype, 
              int dest, 
              int tag, 
              MPI_Comm comm)
{
  int mpi_ret=0;
  PRINT_DEBUG("here inside MPI_Rsend\n");
  PRINT_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cd = CDPath::GetCurrentCD();
  if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    mpi_ret = PMPI_Rsend(buf, count, datatype, dest, tag, comm);

    PRINT_DEBUG("In kGenerateLog mode, generating new logs...\n");
    //cur_cd->ptr_cd()->LogData(&buf, sizeof(buf));
    cur_cd->ptr_cd()->LogData(buf, 0);
  }
  else
  {
    PRINT_DEBUG("In kReplay mode, replaying from logs...\n");
    //CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(&buf, sizeof(buf));
    CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(buf, 0);
    if (ret == kCommLogCommLogModeFlip)
    {
      PRINT_DEBUG("Reached end of logs, and begin to generate logs...\n");
      mpi_ret = PMPI_Rsend(buf, count, datatype, dest, tag, comm);
      //cur_cd->ptr_cd()->LogData(&buf, sizeof(buf));
      cur_cd->ptr_cd()->LogData(buf, 0);
    }
  }

  return mpi_ret;
}

// basic send with user-provided buffering, need buffer attach/detach
// messages are guaranteed to arrive only after buffer detach
// note: this function is thread-safe
int MPI_Bsend(void *buf, 
              int count, 
              MPI_Datatype datatype, 
              int dest, 
              int tag, 
              MPI_Comm comm)
{
  int mpi_ret=0;
  PRINT_DEBUG("here inside MPI_Bsend\n");
  PRINT_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cd = CDPath::GetCurrentCD();
  if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    mpi_ret = PMPI_Bsend(buf, count, datatype, dest, tag, comm);

    PRINT_DEBUG("In kGenerateLog mode, generating new logs...\n");
    //cur_cd->ptr_cd()->LogData(&buf, sizeof(buf));
    cur_cd->ptr_cd()->LogData(buf, 0);
  }
  else
  {
    PRINT_DEBUG("In kReplay mode, replaying from logs...\n");
    //CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(&buf, sizeof(buf));
    CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(buf, 0);
    if (ret == kCommLogCommLogModeFlip)
    {
      PRINT_DEBUG("Reached end of logs, and begin to generate logs...\n");
      mpi_ret = PMPI_Bsend(buf, count, datatype, dest, tag, comm);
      //cur_cd->ptr_cd()->LogData(&buf, sizeof(buf));
      cur_cd->ptr_cd()->LogData(buf, 0);
    }
  }

  return mpi_ret;
}

// block receive
// this function is thread-safe
int MPI_Recv(void *buf, 
             int count, 
             MPI_Datatype datatype, 
             int src, 
             int tag, 
             MPI_Comm comm, 
             MPI_Status *status)
{
  int mpi_ret=0;
  PRINT_DEBUG("here inside MPI_Recv\n");
  PRINT_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cd = CDPath::GetCurrentCD();
  if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    mpi_ret = PMPI_Recv(buf, count, datatype, src, tag, comm, status);

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
      mpi_ret = PMPI_Recv(buf, count, datatype, src, tag, comm, status);
      cur_cd->ptr_cd()->LogData(buf, count*sizeof(datatype));
    }
  }

  return mpi_ret;
}

// send and recv a message
// this function is thread-safe
int MPI_Sendrecv(void *sendbuf, 
                 int sendcount, 
                 MPI_Datatype sendtype, 
                 int dest, 
                 int sendtag, 
                 void *recvbuf, 
                 int recvcount, 
                 MPI_Datatype recvtype, 
                 int source, 
                 int recvtag, 
                 MPI_Comm comm, 
                 MPI_Status *status)
{
  int mpi_ret=0;
  PRINT_DEBUG("here inside MPI_Sendrecv\n");
  PRINT_DEBUG("sendbuf=%p, &sendbuf=%p\n", sendbuf, &sendbuf);
  PRINT_DEBUG("recvbuf=%p, &recvbuf=%p\n", recvbuf, &recvbuf);

  CDHandle * cur_cd = CDPath::GetCurrentCD();
  if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    mpi_ret = PMPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag,
                          recvbuf, recvcount, recvtype, source, recvtag, comm, status);

    PRINT_DEBUG("In kGenerateLog mode, generating new logs...\n");
    //cur_cd->ptr_cd()->LogData(&sendbuf, sizeof(sendbuf));
    cur_cd->ptr_cd()->LogData(sendbuf, 0);
    cur_cd->ptr_cd()->LogData(recvbuf, recvcount*sizeof(recvtype));
  }
  else
  {
    PRINT_DEBUG("In kReplay mode, replaying from logs...\n");
    //CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(&sendbuf, sizeof(sendbuf));
    CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(sendbuf, 0);
    if (ret == kCommLogCommLogModeFlip)
    {
      PRINT_DEBUG("Reached end of logs, and begin to generate logs...\n");
      mpi_ret = PMPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag,
                            recvbuf, recvcount, recvtype, source, recvtag, comm, status);
      //cur_cd->ptr_cd()->LogData(&sendbuf, sizeof(sendbuf));
      cur_cd->ptr_cd()->LogData(sendbuf, 0);
      cur_cd->ptr_cd()->LogData(recvbuf, recvcount*sizeof(recvtype));
    }
    else if (ret == kCommLogOK)
    {
      ret = cur_cd->ptr_cd()->ReadData(recvbuf, recvcount*sizeof(recvtype));
      if (ret == kCommLogCommLogModeFlip)
      {
        PRINT_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag,
                              recvbuf, recvcount, recvtype, source, recvtag, comm, status);
        //FIXME: should we log &sendbuf again in case buf address changes??
        cur_cd->ptr_cd()->LogData(recvbuf, recvcount*sizeof(recvtype));
      }
    }
  }

  return mpi_ret;
}

// send and recv a message using a single buffer
// this function is thread-safe
int MPI_Sendrecv_replace(void *buf, 
                         int count, 
                         MPI_Datatype datatype, 
                         int dest, 
                         int sendtag,
                         int source, 
                         int recvtag, 
                         MPI_Comm comm, 
                         MPI_Status *status)
{
  int mpi_ret=0;
  PRINT_DEBUG("here inside MPI_Recv\n");
  PRINT_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cd = CDPath::GetCurrentCD();
  if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    mpi_ret = PMPI_Sendrecv_replace(buf, count, datatype, dest, sendtag, source, recvtag, comm, status);

    PRINT_DEBUG("In kGenerateLog mode, generating new logs...\n");
    //cur_cd->ptr_cd()->LogData(&buf, sizeof(buf));
    cur_cd->ptr_cd()->LogData(buf, 0);
    cur_cd->ptr_cd()->LogData(buf, count*sizeof(datatype));
  }
  else
  {
    PRINT_DEBUG("In kReplay mode, replaying from logs...\n");
    //CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(&buf, sizeof(buf));
    CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(buf, 0);
    if (ret == kCommLogCommLogModeFlip)
    {
      PRINT_DEBUG("Reached end of logs, and begin to generate logs...\n");
      mpi_ret = PMPI_Sendrecv_replace(buf, count, datatype, dest, sendtag, source, recvtag, comm, status);
      //cur_cd->ptr_cd()->LogData(&buf, sizeof(buf));
      cur_cd->ptr_cd()->LogData(buf, 0);
      cur_cd->ptr_cd()->LogData(buf, count*sizeof(datatype));
    }
    else if (ret == kCommLogOK)
    {
      ret = cur_cd->ptr_cd()->ReadData(buf, count*sizeof(datatype));
      if (ret == kCommLogCommLogModeFlip)
      {
        PRINT_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Sendrecv_replace(buf, count, datatype, dest, sendtag, source, recvtag, comm, status);
        //FIXME: should we log &buf again in case buf address changes??
        cur_cd->ptr_cd()->LogData(buf, count*sizeof(datatype));
      }
    }
  }

  return mpi_ret;
}


// ------------------------------------------------------------------------------------------------------------
// non-blocking p2p communication
// ------------------------------------------------------------------------------------------------------------

// non-blocking send
int MPI_Isend(void *buf, 
              int count, 
              MPI_Datatype datatype, 
              int dest, 
              int tag, 
              MPI_Comm comm, 
              MPI_Request *request)
{
  int mpi_ret=0;
  PRINT_DEBUG("here inside MPI_Isend\n");
  PRINT_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cd = CDPath::GetCurrentCD();
  if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    mpi_ret = PMPI_Isend(buf, count, datatype, dest, tag, comm, request);

    PRINT_DEBUG("In kGenerateLog mode, generating new logs...\n");
    cur_cd->ptr_cd()->LogData(request, 0, false);
  }
  else
  {
    PRINT_DEBUG("In kReplay mode, replaying from logs...\n");
    CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(request, 0);
    if (ret == kCommLogCommLogModeFlip)
    {
      PRINT_DEBUG("Reached end of logs, and begin to generate logs...\n");
      mpi_ret = PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
      cur_cd->ptr_cd()->LogData(request, 0, false);
    }
    else if (ret == kCommLogError)
    {
      ERROR_MESSAGE("Needs to escalate, not implemented yet...\n");
    }
  }

  return mpi_ret;
}

// non-blocking receive
int MPI_Irecv(void *buf, 
              int count, 
              MPI_Datatype datatype, 
              int src, 
              int tag, 
              MPI_Comm comm, 
              MPI_Request *request)
{
  int mpi_ret=0;
  PRINT_DEBUG("here inside MPI_Irecv\n");
  PRINT_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cd = CDPath::GetCurrentCD();
  if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    mpi_ret = PMPI_Irecv(buf, count, datatype, src, tag, comm, request);

    PRINT_DEBUG("In kGenerateLog mode, generating new logs...\n");
    cur_cd->ptr_cd()->LogData(&request, sizeof(request), false);
  }
  else
  {
    PRINT_DEBUG("In kReplay mode, replaying from logs...\n");
    CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(&request, sizeof(request));
    if (ret == kCommLogCommLogModeFlip)
    {
      PRINT_DEBUG("Reached end of logs, and begin to generate logs...\n");
      mpi_ret = PMPI_Irecv(buf, count, datatype, src, tag, comm, request);
      cur_cd->ptr_cd()->LogData(&request, sizeof(request), false);
    }
    else if (ret == kCommLogError)
    {
      ERROR_MESSAGE("Needs to escalate, not implemented yet...\n");
    }
  }

  return mpi_ret;
}

// wait functions 
int MPI_Wait(MPI_Request *request, 
             MPI_Status *status)
{
  int mpi_ret = 0;
  PRINT_DEBUG("here inside MPI_Wait\n");

  CDHandle * cur_cd = CDPath::GetCurrentCD();
  if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    mpi_ret = PMPI_Wait(request, status);

    PRINT_DEBUG("In kGenerateLog mode, generating new logs...\n");
    //TODO: here
    cur_cd->ptr_cd()->ProbeAndLogData(request);
  }
  else
  {
    PRINT_DEBUG("In kReplay mode, replaying from logs...\n");
    CommLogErrT ret = kCommLogOK;
    //CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(&request, sizeof(request));
    if (ret == kCommLogCommLogModeFlip)
    {
      PRINT_DEBUG("Reached end of logs, and begin to generate logs...\n");
      mpi_ret = PMPI_Wait(request, status);
      //cur_cd->ptr_cd()->LogData(&request, sizeof(request), false);
    }
    else if (ret == kCommLogError)
    {
      ERROR_MESSAGE("Needs to escalate, not implemented yet...\n");
    }
  }
  return mpi_ret;
}


// ------------------------------------------------------------------------------------------------------------
// collective communication
// ------------------------------------------------------------------------------------------------------------

// MPI_Barrier
int MPI_Barrier (MPI_Comm comm)
{
  int mpi_ret=0;
  PRINT_DEBUG("here inside MPI_Barrier\n");
  PRINT_DEBUG("comm=%p, &comm=%p\n", comm, &comm);

  CDHandle * cur_cd = CDPath::GetCurrentCD();
  if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    mpi_ret = PMPI_Barrier(comm);

    PRINT_DEBUG("In kGenerateLog mode, generating new logs...\n");
    cur_cd->ptr_cd()->LogData(&comm, 0);
  }
  else
  {
    PRINT_DEBUG("In kReplay mode, replaying from logs...\n");
    CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(&comm, 0);
    if (ret == kCommLogCommLogModeFlip)
    {
      PRINT_DEBUG("Reached end of logs, and begin to generate logs...\n");
      mpi_ret = PMPI_Barrier(comm);
      cur_cd->ptr_cd()->LogData(&comm, 0);
    }
  }

  return mpi_ret;
}

// MPI_Bcast
int MPI_Bcast (void *buffer,
               int count,
               MPI_Datatype datatype,
               int root,
               MPI_Comm comm)
{
  int mpi_ret=0;
  PRINT_DEBUG("here inside MPI_Bcast\n");

  CDHandle * cur_cd = CDPath::GetCurrentCD();
  if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    mpi_ret = PMPI_Bcast(buffer, count, datatype, root, comm);

    PRINT_DEBUG("In kGenerateLog mode, generating new logs...\n");
    cur_cd->ptr_cd()->LogData(buffer, count*sizeof(datatype));
  }
  else
  {
    PRINT_DEBUG("In kReplay mode, replaying from logs...\n");
    CommLogErrT ret = cur_cd->ptr_cd()->ReadData(buffer, count*sizeof(datatype));
    if (ret == kCommLogCommLogModeFlip)
    {
      PRINT_DEBUG("Reached end of logs, and begin to generate logs...\n");
      mpi_ret = PMPI_Bcast(buffer, count, datatype, root, comm);
      cur_cd->ptr_cd()->LogData(buffer, count*sizeof(datatype));
    }
  }

  return mpi_ret;
}

// MPI_Gather
int MPI_Gather(void *sendbuf,
               int sendcnt,
               MPI_Datatype sendtype,
               void *recvbuf,
               int recvcnt,
               MPI_Datatype recvtype,
               int root, 
               MPI_Comm comm)
{
  int mpi_ret = 0;
  int myrank;
  MPI_Comm_rank(comm, &myrank);
  PRINT_DEBUG("myrank=%d\n", myrank);

  CDHandle * cur_cd = CDPath::GetCurrentCD();
  if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    mpi_ret = PMPI_Gather(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);

    PRINT_DEBUG("In kGenerateLog mode, generating new logs...\n");
    if (myrank == root){
      cur_cd->ptr_cd()->LogData(recvbuf, recvcnt*sizeof(recvtype));
    }
    else {
      cur_cd->ptr_cd()->LogData(sendbuf, 0);
    }
  }
  else
  {
    PRINT_DEBUG("In kReplay mode, replaying from logs...\n");
    CommLogErrT ret;
    if (myrank == root){
      ret = cur_cd->ptr_cd()->ReadData(recvbuf, recvcnt*sizeof(recvtype));
    }
    else{
      ret = cur_cd->ptr_cd()->ProbeData(sendbuf, 0);
    }

    if (ret == kCommLogCommLogModeFlip)
    {
      PRINT_DEBUG("Reached end of logs, and begin to generate logs...\n");
      mpi_ret = PMPI_Gather(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
      if (myrank == root) {
        cur_cd->ptr_cd()->LogData(recvbuf, recvcnt*sizeof(recvtype));
      }
      else {
        cur_cd->ptr_cd()->LogData(sendbuf, 0);
      }
    }
  }

  return mpi_ret;
}


// MPI_Gatherv
int MPI_Gatherv(void *sendbuf,
               int sendcnt,
               MPI_Datatype sendtype,
               void *recvbuf,
               int *recvcnts,
               int *displs,
               MPI_Datatype recvtype,
               int root, 
               MPI_Comm comm)
{
  int mpi_ret = 0;
  int myrank, size;
  MPI_Comm_rank(comm, &myrank);
  MPI_Comm_size(comm, &size);
  PRINT_DEBUG("myrank=%d, size=%d\n", myrank, size);

  // calculate length of recv array
  long totalcnts=0;
  int i;
  for (i=0;i<size;i++)
  {
    totalcnts += recvcnts[i];
  }
  for (i=0;i<size;i++)
  {
    if (totalcnts < displs[i]+recvcnts[i])
      totalcnts = displs[i]+recvcnts[i];
  }
  PRINT_DEBUG("totalcnts = %ld\n", totalcnts);

  CDHandle * cur_cd = CDPath::GetCurrentCD();
  if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
  {
    mpi_ret = PMPI_Gatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcnts, displs, recvtype, root, comm);

    PRINT_DEBUG("In kGenerateLog mode, generating new logs...\n");
    if (myrank == root){
      cur_cd->ptr_cd()->LogData(recvbuf, totalcnts*sizeof(recvtype));
    }
    else {
      cur_cd->ptr_cd()->LogData(sendbuf, 0);
    }
  }
  else
  {
    PRINT_DEBUG("In kReplay mode, replaying from logs...\n");
    CommLogErrT ret;
    if (myrank == root){
      ret = cur_cd->ptr_cd()->ReadData(recvbuf, totalcnts*sizeof(recvtype));
    }
    else{
      ret = cur_cd->ptr_cd()->ProbeData(sendbuf, 0);
    }

    if (ret == kCommLogCommLogModeFlip)
    {
      PRINT_DEBUG("Reached end of logs, and begin to generate logs...\n");
      mpi_ret = PMPI_Gatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcnts, displs, recvtype, root, comm);
      if (myrank == root) {
        cur_cd->ptr_cd()->LogData(recvbuf, totalcnts*sizeof(recvtype));
      }
      else {
        cur_cd->ptr_cd()->LogData(sendbuf, 0);
      }
    }
  }

  return mpi_ret;
}


#endif
