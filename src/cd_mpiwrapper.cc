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

#if _MPI_VER

#ifdef comm_log

#include "cd_mpiwrapper.h"
#include "cd_path.h"
#include "cd_global.h"
#include "cd_comm_log.h"

// -------------------------------------------------------------------------------------------------------
// blocking p2p communication
// -------------------------------------------------------------------------------------------------------

// blocking send: this one will return when the buffer is ready to reuse, but not guarantee messages
// have been received, because small messages may be copied into internal buffers
// this function is thread-safe
int MPI_Send(const void *buf, 
             int count, 
             MPI_Datatype datatype, 
             int dest, 
             int tag, 
             MPI_Comm comm)
{
  app_side = false;
  int mpi_ret=0;
  LOG_DEBUG("here inside MPI_Send\n");
  LOG_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Send(const_cast<void *>(buf), count, datatype, dest, tag, comm);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      //cur_cd->ptr_cd()->LogData(&buf, sizeof(buf));
      cur_cd->ptr_cd()->LogData(buf, 0);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(buf, 0);
      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Send(const_cast<void *>(buf), count, datatype, dest, tag, comm);
        //cur_cd->ptr_cd()->LogData(&buf, sizeof(buf));
        cur_cd->ptr_cd()->LogData(buf, 0);
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Send(const_cast<void *>(buf), count, datatype, dest, tag, comm);
  }

  app_side = true;
  return mpi_ret;
}

// synchronous send: the send will complete until the corresponding receive has been posted
// note: this function is thread-safe
int MPI_Ssend(const void *buf, 
              int count, 
              MPI_Datatype datatype, 
              int dest, 
              int tag, 
              MPI_Comm comm)
{
  app_side = false;
  int mpi_ret=0;
  LOG_DEBUG("here inside MPI_Ssend\n");
  LOG_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Ssend(const_cast<void *>(buf), count, datatype, dest, tag, comm);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      //cur_cd->ptr_cd()->LogData(&buf, sizeof(buf));
      cur_cd->ptr_cd()->LogData(buf, 0);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(buf, 0);
      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Ssend(const_cast<void *>(buf), count, datatype, dest, tag, comm);
        //cur_cd->ptr_cd()->LogData(&buf, sizeof(buf));
        cur_cd->ptr_cd()->LogData(buf, 0);
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Ssend(const_cast<void *>(buf), count, datatype, dest, tag, comm);
  }

  app_side = true;
  return mpi_ret;
}

// ready send: this function can only be called when user is sure receive call has been posted
// this function may save some handshakes for communication in some MPI implementations
// note: this function is thread-safe
int MPI_Rsend(const void *buf, 
              int count, 
              MPI_Datatype datatype, 
              int dest, 
              int tag, 
              MPI_Comm comm)
{
  app_side = false;
  int mpi_ret=0;
  LOG_DEBUG("here inside MPI_Rsend\n");
  LOG_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Rsend(const_cast<void *>(buf), count, datatype, dest, tag, comm);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      //cur_cd->ptr_cd()->LogData(&buf, sizeof(buf));
      cur_cd->ptr_cd()->LogData(buf, 0);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(buf, 0);
      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Rsend(const_cast<void *>(buf), count, datatype, dest, tag, comm);
        //cur_cd->ptr_cd()->LogData(&buf, sizeof(buf));
        cur_cd->ptr_cd()->LogData(buf, 0);
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Rsend(const_cast<void *>(buf), count, datatype, dest, tag, comm);
  }

  app_side = true;
  return mpi_ret;
}

// basic send with user-provided buffering, need buffer attach/detach
// messages are guaranteed to arrive only after buffer detach
// note: this function is thread-safe
int MPI_Bsend(const void *buf, 
              int count, 
              MPI_Datatype datatype, 
              int dest, 
              int tag, 
              MPI_Comm comm)
{
  app_side = false;
  int mpi_ret=0;
  LOG_DEBUG("here inside MPI_Bsend\n");
  LOG_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Bsend(const_cast<void *>(buf), count, datatype, dest, tag, comm);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      //cur_cd->ptr_cd()->LogData(&buf, sizeof(buf));
      cur_cd->ptr_cd()->LogData(buf, 0);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(buf, 0);
      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Bsend(const_cast<void *>(buf), count, datatype, dest, tag, comm);
        //cur_cd->ptr_cd()->LogData(&buf, sizeof(buf));
        cur_cd->ptr_cd()->LogData(buf, 0);
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Bsend(const_cast<void *>(buf), count, datatype, dest, tag, comm);
  }

  app_side = true;
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
  app_side = false;
  int mpi_ret=0;
  int type_size;
  MPI_Type_size(datatype, &type_size);

  LOG_DEBUG("here inside MPI_Recv\n");
  LOG_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Recv(const_cast<void *>(buf), count, datatype, src, tag, comm, status);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cd->ptr_cd()->LogData(buf, count*type_size);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ReadData(buf, count*type_size);
      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Recv(const_cast<void *>(buf), count, datatype, src, tag, comm, status);
        cur_cd->ptr_cd()->LogData(buf, count*type_size);
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Recv(const_cast<void *>(buf), count, datatype, src, tag, comm, status);
  }

  app_side = true;
  return mpi_ret;
}

// send and recv a message
// this function is thread-safe
int MPI_Sendrecv(const void *sendbuf, 
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
  app_side = false;
  int mpi_ret=0;
  int type_size;
  MPI_Type_size(recvtype, &type_size);
  LOG_DEBUG("here inside MPI_Sendrecv\n");
  LOG_DEBUG("sendbuf=%p, &sendbuf=%p\n", sendbuf, &sendbuf);
  LOG_DEBUG("recvbuf=%p, &recvbuf=%p\n", recvbuf, &recvbuf);

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Sendrecv(const_cast<void *>(sendbuf), sendcount, sendtype, dest, sendtag,
                            recvbuf, recvcount, recvtype, source, recvtag, comm, status);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      //cur_cd->ptr_cd()->LogData(&sendbuf, sizeof(sendbuf));
      cur_cd->ptr_cd()->LogData(sendbuf, 0);
      cur_cd->ptr_cd()->LogData(recvbuf, recvcount*type_size);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(sendbuf, 0);
      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Sendrecv(const_cast<void *>(sendbuf), sendcount, sendtype, dest, sendtag,
                              recvbuf, recvcount, recvtype, source, recvtag, comm, status);
        //cur_cd->ptr_cd()->LogData(&sendbuf, sizeof(sendbuf));
        cur_cd->ptr_cd()->LogData(sendbuf, 0);
        cur_cd->ptr_cd()->LogData(recvbuf, recvcount*type_size);
      }
      else if (ret == kCommLogOK)
      {
        ret = cur_cd->ptr_cd()->ReadData(recvbuf, recvcount*type_size);
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Sendrecv(const_cast<void *>(sendbuf), sendcount, sendtype, dest, sendtag,
                                recvbuf, recvcount, recvtype, source, recvtag, comm, status);
          //FIXME: should we log &sendbuf again in case buf address changes??
          cur_cd->ptr_cd()->LogData(recvbuf, recvcount*type_size);
        }
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Sendrecv(const_cast<void *>(sendbuf), sendcount, sendtype, dest, sendtag,
                            recvbuf, recvcount, recvtype, source, recvtag, comm, status);
  }

  app_side = true;
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
  app_side = false;
  int mpi_ret=0;
  int type_size;
  MPI_Type_size(datatype, &type_size);
  LOG_DEBUG("here inside MPI_Recv\n");
  LOG_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Sendrecv_replace(buf, count, datatype, dest, sendtag, source, recvtag, comm, status);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      //cur_cd->ptr_cd()->LogData(&buf, sizeof(buf));
      cur_cd->ptr_cd()->LogData(buf, 0);
      cur_cd->ptr_cd()->LogData(buf, count*type_size);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(buf, 0);
      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Sendrecv_replace(buf, count, datatype, dest, sendtag, source, recvtag, comm, status);
        //cur_cd->ptr_cd()->LogData(&buf, sizeof(buf));
        cur_cd->ptr_cd()->LogData(buf, 0);
        cur_cd->ptr_cd()->LogData(buf, count*type_size);
      }
      else if (ret == kCommLogOK)
      {
        ret = cur_cd->ptr_cd()->ReadData(buf, count*type_size);
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Sendrecv_replace(buf, count, datatype, dest, sendtag, source, recvtag, comm, status);
          //FIXME: should we log &buf again in case buf address changes??
          cur_cd->ptr_cd()->LogData(buf, count*type_size);
        }
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Sendrecv_replace(buf, count, datatype, dest, sendtag, source, recvtag, comm, status);
  }

  app_side = true;
  return mpi_ret;
}


// -------------------------------------------------------------------------------------------------------
// non-blocking p2p communication
// -------------------------------------------------------------------------------------------------------

// non-blocking send
int MPI_Isend(const void *buf, 
              int count, 
              MPI_Datatype datatype, 
              int dest, 
              int tag, 
              MPI_Comm comm, 
              MPI_Request *request)
{
  app_side = false;
  int mpi_ret=0;
  LOG_DEBUG("here inside MPI_Isend\n");
  LOG_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Isend(const_cast<void *>(buf), count, datatype, dest, tag, comm, request);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cd->ptr_cd()->LogData(buf, 0, false, (unsigned long)request, 0);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(buf, 0);
      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Isend(const_cast<void *>(buf), count, datatype, dest, tag, comm, request);
        cur_cd->ptr_cd()->LogData(buf, 0, false, (unsigned long)request, 0);
      }
      else if (ret == kCommLogError)
      {
        ERROR_MESSAGE("Needs to escalate, not implemented yet...\n");
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Isend(const_cast<void *>(buf), count, datatype, dest, tag, comm, request);
  }

  app_side = true;
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
  app_side = false;
  int mpi_ret=0;
  int type_size;
  MPI_Type_size(datatype, &type_size);
  LOG_DEBUG("here inside MPI_Irecv\n");
  LOG_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cd = GetCurrentCD();

  printf( "\n\n cd mode : %d\n\n\n", cur_cd->ptr_cd()->GetCDType());

  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Irecv(buf, count, datatype, src, tag, comm, request);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cd->ptr_cd()->LogData(buf, count*type_size, false, (unsigned long)request, 1);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ReadData(buf, count*type_size);
      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Irecv(buf, count, datatype, src, tag, comm, request);
        cur_cd->ptr_cd()->LogData(buf, count*type_size, false, (unsigned long)request, 1);
      }
      else if (ret == kCommLogError)
      {
        ERROR_MESSAGE("Needs to escalate, not implemented yet...\n");
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Irecv(buf, count, datatype, src, tag, comm, request);
  }

  app_side = true;
  return mpi_ret;
}

// test functions 
int MPI_Test(MPI_Request *request, 
             int * flag,
             MPI_Status *status)
{
  app_side = false;
  int mpi_ret = 0;
  LOG_DEBUG("here inside MPI_Test\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Test(request, flag, status);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      if (*flag == 1)
      {
        LOG_DEBUG("Operation complete, log flag and data...\n");
        cur_cd->ptr_cd()->LogData(flag, sizeof(int));
        cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)request);
      }
      else
      {
        LOG_DEBUG("Operation not complete, log flag...\n");
        cur_cd->ptr_cd()->LogData(flag, sizeof(int), true, 0, false, true);
      }
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ReadData(flag, sizeof(int));
      if (ret == kCommLogOK)
      {
        if (*flag == 1)
        {
          cur_cd->ptr_cd()->ProbeData(request, 0);
        }
      }
      else if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Test(request, flag, status);
        if (*flag == 1)
        {
          LOG_DEBUG("Operation complete, log flag and data...\n");
          cur_cd->ptr_cd()->LogData(flag, sizeof(int));
          cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)request);
        }
        else
        {
          LOG_DEBUG("Operation not complete, log flag...\n");
          cur_cd->ptr_cd()->LogData(flag, sizeof(int), true, 0, false, true);
        }
      }
      else if (ret == kCommLogError)
      {
        ERROR_MESSAGE("Needs to escalate, not implemented yet...\n");
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Test(request, flag, status);
    // delete incomplete entries...
    if (*flag == 1)
    {
      cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)request);
    }
  }

  app_side = true;
  return mpi_ret;
}


int MPI_Testall(int count,
                MPI_Request array_of_requests[],
                int *flag,
                MPI_Status array_of_statuses[])
{
  app_side = false;
  int mpi_ret = 0;
  LOG_DEBUG("here inside MPI_Testall\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Testall(count, array_of_requests, flag, array_of_statuses);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      if (*flag == 1)
      {
        LOG_DEBUG("Operation complete, log flag and data...\n");
        cur_cd->ptr_cd()->LogData(flag, sizeof(int));
        for (int ii=0;ii<count;ii++)
        {
          cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[ii]);
        }
      }
      else
      {
        LOG_DEBUG("Operation not complete, log flag...\n");
        cur_cd->ptr_cd()->LogData(flag, sizeof(int), true, 0, false, true);
      }
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ReadData(flag, sizeof(int));
      if (ret == kCommLogOK)
      {
        if (*flag == 1)
        {
          for (int ii=0;ii<count;ii++)
          {
            cur_cd->ptr_cd()->ProbeData(&array_of_requests[ii], 0);
          }
        }
      }
      else if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Testall(count, array_of_requests, flag, array_of_statuses);
        if (*flag == 1)
        {
          LOG_DEBUG("Operation complete, log flag and data...\n");
          cur_cd->ptr_cd()->LogData(flag, sizeof(int));
          for (int ii=0;ii<count;ii++)
          {
            cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[ii]);
          }
        }
        else
        {
          LOG_DEBUG("Operation not complete, log flag...\n");
          cur_cd->ptr_cd()->LogData(flag, sizeof(int), true, 0, false, true);
        }
      }
      else if (ret == kCommLogError)
      {
        ERROR_MESSAGE("Needs to escalate, not implemented yet...\n");
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Testall(count, array_of_requests, flag, array_of_statuses);
    // delete incomplete entries...
    if (*flag == 1)
    {
      for (int ii=0;ii<count;ii++)
      {
        cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[ii]);
      }
    }
  }

  app_side = true;
  return mpi_ret;
}

int MPI_Testany(int count,
                MPI_Request array_of_requests[],
                int *index,
                int *flag,
                MPI_Status *status)
{
  app_side = false;
  int mpi_ret = 0;
  LOG_DEBUG("here inside MPI_Testany\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Testany(count, array_of_requests, index, flag, status);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      if (*flag == 1)
      {
        LOG_DEBUG("Operation complete, log flag and data...\n");
        cur_cd->ptr_cd()->LogData(flag, sizeof(int));
        cur_cd->ptr_cd()->LogData(index, sizeof(int));
        cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[*index]);
      }
      else
      {
        LOG_DEBUG("Operation not complete, log flag...\n");
        cur_cd->ptr_cd()->LogData(flag, sizeof(int), true, 0, false, true);
      }
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ReadData(flag, sizeof(int));
      if (ret == kCommLogOK)
      {
        if (*flag == 1)
        {
          cur_cd->ptr_cd()->ReadData(index, sizeof(int));
          cur_cd->ptr_cd()->ProbeData(&array_of_requests[*index], 0);
        }
      }
      else if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Testany(count, array_of_requests, index, flag, status);
        if (*flag == 1)
        {
          LOG_DEBUG("Operation complete, log flag and data...\n");
          cur_cd->ptr_cd()->LogData(flag, sizeof(int));
          cur_cd->ptr_cd()->LogData(index, sizeof(int));
          cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[*index]);
        }
        else
        {
          LOG_DEBUG("Operation not complete, log flag...\n");
          cur_cd->ptr_cd()->LogData(flag, sizeof(int), true, 0, false, true);
        }
      }
      else if (ret == kCommLogError)
      {
        ERROR_MESSAGE("Needs to escalate, not implemented yet...\n");
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Testany(count, array_of_requests, index, flag, status);
    // delete incomplete entries...
    if (*flag == 1)
    {
      cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[*index]);
    }
  }

  app_side = true;
  return mpi_ret;
}

int MPI_Testsome(int incount,
                 MPI_Request array_of_requests[],
                 int *outcount,
                 int array_of_indices[],
                 MPI_Status array_of_statuses[])
{
  app_side = false;
  int mpi_ret = 0;
  LOG_DEBUG("here inside MPI_Testsome\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      if (*outcount > 0)
      {
        LOG_DEBUG("Operation complete, log outcount and data...\n");
        cur_cd->ptr_cd()->LogData(outcount, sizeof(int));
        for (int ii=0;ii<*outcount;ii++)
        {
          cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[array_of_indices[ii]]);
        }
      }
      else
      {
        LOG_DEBUG("Operation not complete, log outcount...\n");
        cur_cd->ptr_cd()->LogData(outcount, sizeof(int), true, 0, false, true);
      }
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ReadData(outcount, sizeof(int));
      if (ret == kCommLogOK)
      {
        if (*outcount > 0)
        {
          for (int ii=0;ii<*outcount;ii++)
          {
            cur_cd->ptr_cd()->ProbeData(&array_of_requests[array_of_indices[ii]], 0);
          }
        }
      }
      else if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
        if (*outcount > 0)
        {
          LOG_DEBUG("Operation complete, log outcount and data...\n");
          cur_cd->ptr_cd()->LogData(outcount, sizeof(int));
          for (int ii=0;ii<*outcount;ii++)
          {
            cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[array_of_indices[ii]]);
          }
        }
        else
        {
          LOG_DEBUG("Operation not complete, log outcount...\n");
          cur_cd->ptr_cd()->LogData(outcount, sizeof(int), true, 0, false, true);
        }
      }
      else if (ret == kCommLogError)
      {
        ERROR_MESSAGE("Needs to escalate, not implemented yet...\n");
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
    // delete incomplete entries...
    if (*outcount > 0)
    {
      for (int ii=0;ii<*outcount;ii++)
      {
        cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[array_of_indices[ii]]);
      }
    }
  }

  app_side = true;
  return mpi_ret;
}

// wait functions 
int MPI_Wait(MPI_Request *request, 
             MPI_Status *status)
{
  app_side = false;
  int mpi_ret = 0;
  LOG_DEBUG("here inside MPI_Wait\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Wait(request, status);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)request);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(request,0);
      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        LOG_DEBUG("Should not come here because error happens between Isend/Irecv and WaitXXX...\n");
        mpi_ret = PMPI_Wait(request, status);
        cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)request);
      }
      else if (ret == kCommLogError)
      {
        ERROR_MESSAGE("Needs to escalate, not implemented yet...\n");
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Wait(request, status);
    // delete incomplete entries...
    cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)request);
  }

  app_side = true;
  return mpi_ret;
}

// wait functions 
int MPI_Waitall(int count, MPI_Request array_of_requests[], 
                MPI_Status array_of_statuses[])
{
  app_side = false;
  int mpi_ret = 0;
  int ii=0;
  LOG_DEBUG("here inside MPI_Waitall\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Waitall(count, array_of_requests, array_of_statuses);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      for (ii=0;ii<count;ii++)
      {
        cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[ii]);
      }
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret;
      for (ii=0;ii<count;ii++)
      {
        ret = cur_cd->ptr_cd()->ProbeData(&array_of_requests[ii],0);
        if (ret == kCommLogCommLogModeFlip) 
        {
          if (ii != 0)
          {
            ERROR_MESSAGE("Partially instrumented MPI_Waitall, may cause incorrect re-execution!!\n");
          }
          break;
        }
      }

      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        LOG_DEBUG("Should not come here because error happens between Isend/Irecv and WaitXXX...\n");
        mpi_ret = PMPI_Waitall(count, array_of_requests, array_of_statuses);
        for (ii=0;ii<count;ii++)
        {
          cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[ii]);
        }
      }
      else if (ret == kCommLogError)
      {
        ERROR_MESSAGE("Needs to escalate, not implemented yet...\n");
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Waitall(count, array_of_requests, array_of_statuses);
    // delete incomplete entries...
    for (ii=0;ii<count;ii++)
    {
      cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[ii]);
    }
  }

  app_side = true;
  return mpi_ret;
}

// wait functions 
int MPI_Waitany(int count, MPI_Request *array_of_requests, 
                int *index, MPI_Status *status)
{
  app_side = false;
  int mpi_ret = 0;
  LOG_DEBUG("here inside MPI_Waitany\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Waitany(count, array_of_requests, index, status);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cd->ptr_cd()->LogData(index, sizeof(int));
      cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[*index]);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ReadData(index, sizeof(int));
      if (ret == kCommLogOK)
      {
        ret = cur_cd->ptr_cd()->ProbeData(&array_of_requests[*index],0);
      }
      else if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        LOG_DEBUG("Should not come here because error happens between Isend/Irecv and WaitXXX...\n");
        mpi_ret = PMPI_Waitany(count, array_of_requests, index, status);
        cur_cd->ptr_cd()->LogData(index, sizeof(int));
        cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[*index]);
      }
      else if (ret == kCommLogError)
      {
        ERROR_MESSAGE("Needs to escalate, not implemented yet...\n");
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Waitany(count, array_of_requests, index, status);
    // delete incomplete entries...
    cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[*index]);
  }

  app_side = true;
  return mpi_ret;
}

int MPI_Waitsome(int incount,
                 MPI_Request array_of_requests[],
                 int *outcount,
                 int array_of_indices[],
                 MPI_Status array_of_statuses[])
{
  app_side = false;
  int mpi_ret = 0;
  int ii=0;
  LOG_DEBUG("here inside MPI_Waitsome\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cd->ptr_cd()->LogData(outcount, sizeof(int));
      cur_cd->ptr_cd()->LogData(array_of_indices, *outcount*sizeof(int));
      for (ii=0;ii<*outcount;ii++)
      {
        cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[array_of_indices[ii]]);
      }
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ReadData(outcount, sizeof(int));
      if (ret == kCommLogOK)
      {
        cur_cd->ptr_cd()->ReadData(array_of_indices, *outcount*sizeof(int));
        for (ii=0;ii<*outcount;ii++)
        {
          cur_cd->ptr_cd()->ReadData(&array_of_requests[array_of_indices[ii]], 0);
        }
      }
      else if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        LOG_DEBUG("Should not come here because error happens between Isend/Irecv and WaitXXX...\n");
        mpi_ret = PMPI_Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
        cur_cd->ptr_cd()->LogData(outcount, sizeof(int));
        cur_cd->ptr_cd()->LogData(array_of_indices, *outcount*sizeof(int));
        for (ii=0;ii<*outcount;ii++)
        {
          cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[array_of_indices[ii]]);
        }
      }
      else if (ret == kCommLogError)
      {
        ERROR_MESSAGE("Needs to escalate, not implemented yet...\n");
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
    // delete incomplete entries...
    for (ii=0;ii<*outcount;ii++)
    {
      cur_cd->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[array_of_indices[ii]]);
    }
  }

  app_side = true;
  return mpi_ret;
}

// -------------------------------------------------------------------------------------------------------
// collective communication
// -------------------------------------------------------------------------------------------------------

// MPI_Barrier
int MPI_Barrier (MPI_Comm comm)
{
  app_side = false;
  int mpi_ret=0;
  LOG_DEBUG("here inside MPI_Barrier\n");
//  LOG_DEBUG("comm=%x, &comm=%p\n", comm, &comm);

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Barrier(comm);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cd->ptr_cd()->LogData(&comm, 0);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ProbeData(&comm, 0);
      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Barrier(comm);
        cur_cd->ptr_cd()->LogData(&comm, 0);
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Barrier(comm);
  }

  app_side = true;
  return mpi_ret;
}

// MPI_Bcast
int MPI_Bcast (void *buffer,
               int count,
               MPI_Datatype datatype,
               int root,
               MPI_Comm comm)
{
  app_side = false;
  int mpi_ret=0;
  int type_size;
  MPI_Type_size(datatype, &type_size);
  LOG_DEBUG("here inside MPI_Bcast\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Bcast(buffer, count, datatype, root, comm);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cd->ptr_cd()->LogData(buffer, count*type_size);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ReadData(buffer, count*type_size);
      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Bcast(buffer, count, datatype, root, comm);
        cur_cd->ptr_cd()->LogData(buffer, count*type_size);
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Bcast(buffer, count, datatype, root, comm);
  }

  app_side = true;
  return mpi_ret;
}

// MPI_Gather
int MPI_Gather(const void *sendbuf,
               int sendcnt,
               MPI_Datatype sendtype,
               void *recvbuf,
               int recvcnt,
               MPI_Datatype recvtype,
               int root, 
               MPI_Comm comm)
{
  app_side = false;
  int mpi_ret = 0;
  int type_size;
  MPI_Type_size(recvtype, &type_size);
  LOG_DEBUG("here inside MPI_Gather\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    int myrank, size;
    MPI_Comm_rank(comm, &myrank);
    MPI_Comm_size(comm, &size);
    LOG_DEBUG("myrank=%d, size=%d\n", myrank, size);
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Gather(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      if (myrank == root){
        cur_cd->ptr_cd()->LogData(recvbuf, size*recvcnt*type_size);
      }
      else {
        cur_cd->ptr_cd()->LogData(sendbuf, 0);
      }
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret;
      if (myrank == root){
        ret = cur_cd->ptr_cd()->ReadData(recvbuf, size*recvcnt*type_size);
      }
      else{
        ret = cur_cd->ptr_cd()->ProbeData(sendbuf, 0);
      }

      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Gather(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
        if (myrank == root) {
          cur_cd->ptr_cd()->LogData(recvbuf, size*recvcnt*type_size);
        }
        else {
          cur_cd->ptr_cd()->LogData(sendbuf, 0);
        }
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Gather(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
  }

  app_side = true;
  return mpi_ret;
}


// MPI_Gatherv
int MPI_Gatherv(const void *sendbuf,
                int sendcnt,
                MPI_Datatype sendtype,
                void *recvbuf,
                const int *recvcnts,
                const int *displs,
                MPI_Datatype recvtype,
                int root, 
                MPI_Comm comm)
{
  app_side = false;
  int mpi_ret = 0;
  int type_size;
  MPI_Type_size(recvtype, &type_size);
  LOG_DEBUG("here inside MPI_Gatherv\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    int myrank, size;
    MPI_Comm_rank(comm, &myrank);
    MPI_Comm_size(comm, &size);
    LOG_DEBUG("myrank=%d, size=%d\n", myrank, size);

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
    LOG_DEBUG("totalcnts = %ld\n", totalcnts);

    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Gatherv(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, const_cast<int *>(recvcnts), const_cast<int *>(displs), recvtype, root, comm);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      if (myrank == root){
        cur_cd->ptr_cd()->LogData(recvbuf, totalcnts*type_size);
      }
      else {
        cur_cd->ptr_cd()->LogData(sendbuf, 0);
      }
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret;
      if (myrank == root){
        ret = cur_cd->ptr_cd()->ReadData(recvbuf, totalcnts*type_size);
      }
      else{
        ret = cur_cd->ptr_cd()->ProbeData(sendbuf, 0);
      }

      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Gatherv(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, const_cast<int *>(recvcnts), const_cast<int *>(displs), recvtype, root, comm);
        if (myrank == root) {
          cur_cd->ptr_cd()->LogData(recvbuf, totalcnts*type_size);
        }
        else {
          cur_cd->ptr_cd()->LogData(sendbuf, 0);
        }
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Gatherv(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, const_cast<int *>(recvcnts), const_cast<int *>(displs), recvtype, root, comm);
  }

  app_side = true;
  return mpi_ret;
}


// MPI_Allgather
int MPI_Allgather(const void *sendbuf,
                  int sendcnt,
                  MPI_Datatype sendtype,
                  void *recvbuf,
                  int recvcnt,
                  MPI_Datatype recvtype,
                  MPI_Comm comm)
{
  app_side = false;
  int mpi_ret = 0;
  int type_size;
  MPI_Type_size(recvtype, &type_size);
  LOG_DEBUG("here inside MPI_Allgather\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    int size;
    MPI_Comm_size(comm, &size);
    LOG_DEBUG("size=%d\n", size);

    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Allgather(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, recvcnt, recvtype, comm);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cd->ptr_cd()->LogData(recvbuf, size*recvcnt*type_size);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ReadData(recvbuf, size*recvcnt*sizeof(recvtype));
      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Allgather(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, recvcnt, recvtype, comm);
        cur_cd->ptr_cd()->LogData(recvbuf, size*recvcnt*type_size);
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Allgather(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, recvcnt, recvtype, comm);
  }

  app_side = true;
  return mpi_ret;
}


// MPI_Allgatherv
int MPI_Allgatherv(const void *sendbuf,
                   int sendcnt,
                   MPI_Datatype sendtype,
                   void *recvbuf,
                   const int *recvcnts,
                   const int *displs,
                   MPI_Datatype recvtype,
                   MPI_Comm comm)
{
  app_side = false;
  int mpi_ret = 0;
  int type_size;
  MPI_Type_size(recvtype, &type_size);
  LOG_DEBUG("here inside MPI_Allgatherv\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    int size;
    MPI_Comm_size(comm, &size);
    LOG_DEBUG("size=%d\n", size);

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
    LOG_DEBUG("totalcnts = %ld\n", totalcnts);

    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Allgatherv(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, const_cast<int *>(recvcnts), const_cast<int *>(displs), recvtype, comm);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cd->ptr_cd()->LogData(recvbuf, totalcnts*type_size);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ReadData(recvbuf, totalcnts*type_size);

      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Allgatherv(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, const_cast<int *>(recvcnts), const_cast<int *>(displs), recvtype, comm);

        cur_cd->ptr_cd()->LogData(recvbuf, totalcnts*type_size);
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Allgatherv(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, const_cast<int *>(recvcnts), const_cast<int *>(displs), recvtype, comm);
  }

  app_side = true;
  return mpi_ret;
}


// MPI_Reduce
int MPI_Reduce(const void *sendbuf,
               void *recvbuf, 
               int count,
               MPI_Datatype datatype,
               MPI_Op op, 
               int root,
               MPI_Comm comm)
{
  app_side = false;
  int mpi_ret = 0;
  int type_size;
  MPI_Type_size(datatype, &type_size);
  LOG_DEBUG("here inside MPI_Reduce\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    int myrank, size;
    MPI_Comm_rank(comm, &myrank);
    MPI_Comm_size(comm, &size);
    LOG_DEBUG("myrank=%d, size=%d\n", myrank, size);

    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Reduce(const_cast<void *>(sendbuf), recvbuf, count, datatype, op, root, comm);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      if (myrank == root){
        cur_cd->ptr_cd()->LogData(recvbuf, count*type_size);
      }
      else {
        cur_cd->ptr_cd()->LogData(sendbuf, 0);
      }
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret;
      if (myrank == root){
        ret = cur_cd->ptr_cd()->ReadData(recvbuf, count*type_size);
      }
      else{
        ret = cur_cd->ptr_cd()->ProbeData(sendbuf, 0);
      }

      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Reduce(const_cast<void *>(sendbuf), recvbuf, count, datatype, op, root, comm);
        if (myrank == root) {
          cur_cd->ptr_cd()->LogData(recvbuf, count*type_size);
        }
        else {
          cur_cd->ptr_cd()->LogData(sendbuf, 0);
        }
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Reduce(const_cast<void *>(sendbuf), recvbuf, count, datatype, op, root, comm);
  }

  app_side = true;
  return mpi_ret;
}


// MPI_Allreduce
int MPI_Allreduce(const void *sendbuf,
                  void *recvbuf, 
                  int count,
                  MPI_Datatype datatype,
                  MPI_Op op, 
                  MPI_Comm comm)
{
  app_side = false;
  int mpi_ret = 0;
  int type_size;
  MPI_Type_size(datatype, &type_size);
  LOG_DEBUG("here inside MPI_Allreduce\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Allreduce(const_cast<void *>(sendbuf), recvbuf, count, datatype, op, comm);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cd->ptr_cd()->LogData(recvbuf, count*type_size);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ReadData(recvbuf, count*type_size);

      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Allreduce(const_cast<void *>(sendbuf), recvbuf, count, datatype, op, comm);
        cur_cd->ptr_cd()->LogData(recvbuf, count*type_size);
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Allreduce(const_cast<void *>(sendbuf), recvbuf, count, datatype, op, comm);
  }

  app_side = true;
  return mpi_ret;
}


// MPI_Alltoall
int MPI_Alltoall(const void *sendbuf,
                 int sendcnt,
                 MPI_Datatype sendtype,
                 void *recvbuf,
                 int recvcnt,
                 MPI_Datatype recvtype,
                 MPI_Comm comm)
{
  app_side = false;
  int mpi_ret = 0;
  int type_size;
  MPI_Type_size(recvtype, &type_size);
  LOG_DEBUG("here inside MPI_Alltoall\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    int size;
    MPI_Comm_size(comm, &size);
    LOG_DEBUG("size=%d\n", size);

    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Alltoall(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, recvcnt, recvtype, comm);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cd->ptr_cd()->LogData(recvbuf, size*recvcnt*type_size);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ReadData(recvbuf, size*recvcnt*type_size);
      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Alltoall(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, recvcnt, recvtype, comm);
        cur_cd->ptr_cd()->LogData(recvbuf, size*recvcnt*type_size);
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Alltoall(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, recvcnt, recvtype, comm);
  }

  app_side = true;
  return mpi_ret;
}


// MPI_Alltoallv
int MPI_Alltoallv(const void *sendbuf,
                  const int *sendcnts,
                  const int *sdispls,
                  MPI_Datatype sendtype,
                  void *recvbuf,
                  const int *recvcnts,
                  const int *rdispls,
                  MPI_Datatype recvtype,
                  MPI_Comm comm)
{
  app_side = false;
  int mpi_ret = 0;
  int type_size;
  MPI_Type_size(recvtype, &type_size);
  LOG_DEBUG("here inside MPI_Alltoallv\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    int size;
    MPI_Comm_size(comm, &size);
    LOG_DEBUG("size=%d\n", size);

    // calculate length of recv array
    long totalcnts=0;
    int i;
    for (i=0;i<size;i++)
    {
      totalcnts += recvcnts[i];
    }
    LOG_DEBUG("totalcnts = %ld\n", totalcnts);
    for (i=0;i<size;i++)
    {
      if (totalcnts < rdispls[i]+recvcnts[i])
        totalcnts = rdispls[i]+recvcnts[i];
    }
    LOG_DEBUG("totalcnts = %ld\n", totalcnts);

    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Alltoallv(const_cast<void *>(sendbuf), const_cast<int *>(sendcnts), const_cast<int *>(sdispls), sendtype, 
                               recvbuf, const_cast<int *>(recvcnts), const_cast<int *>(rdispls), recvtype, comm);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cd->ptr_cd()->LogData(recvbuf, totalcnts*type_size);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ReadData(recvbuf, totalcnts*type_size);

      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Alltoallv(const_cast<void *>(sendbuf), const_cast<int *>(sendcnts), const_cast<int *>(sdispls), sendtype, 
                                 recvbuf, const_cast<int *>(recvcnts), const_cast<int *>(rdispls), recvtype, comm);

        cur_cd->ptr_cd()->LogData(recvbuf, totalcnts*type_size);
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Alltoallv(const_cast<void *>(sendbuf), const_cast<int *>(sendcnts), const_cast<int *>(sdispls), sendtype, 
                             recvbuf, const_cast<int *>(recvcnts), const_cast<int *>(rdispls), recvtype, comm);
  }

  app_side = true;
  return mpi_ret;
}


// MPI_Scatter
int MPI_Scatter(const void *sendbuf,
                int sendcnt,
                MPI_Datatype sendtype,
                void *recvbuf,
                int recvcnt,
                MPI_Datatype recvtype,
                int root, 
                MPI_Comm comm)
{
  app_side = false;
  int mpi_ret = 0;
  int type_size;
  MPI_Type_size(recvtype, &type_size);
  LOG_DEBUG("here inside MPI_Scatter\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Scatter(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cd->ptr_cd()->LogData(recvbuf, recvcnt*type_size);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ReadData(recvbuf, recvcnt*type_size);

      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Scatter(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
        cur_cd->ptr_cd()->LogData(recvbuf, recvcnt*type_size);
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Scatter(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
  }

  app_side = true;
  return mpi_ret;
}


// MPI_Scatterv
int MPI_Scatterv(const void *sendbuf,
                 const int *sendcnts,
                 const int *displs,
                 MPI_Datatype sendtype,
                 void *recvbuf,
                 int recvcnt,
                 MPI_Datatype recvtype,
                 int root, 
                 MPI_Comm comm)
{
  app_side = false;
  int mpi_ret = 0;
  int type_size;
  MPI_Type_size(recvtype, &type_size);
  LOG_DEBUG("here inside MPI_Scatterv\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Scatterv(const_cast<void *>(sendbuf), const_cast<int *>(sendcnts), const_cast<int *>(displs), sendtype, 
                              recvbuf, recvcnt, recvtype, root, comm);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cd->ptr_cd()->LogData(recvbuf, recvcnt*type_size);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ReadData(recvbuf, recvcnt*type_size);

      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Scatterv(const_cast<void *>(sendbuf), const_cast<int *>(sendcnts), const_cast<int *>(displs), sendtype, 
                                recvbuf, recvcnt, recvtype, root, comm);
        cur_cd->ptr_cd()->LogData(recvbuf, recvcnt*type_size);
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Scatterv(const_cast<void *>(sendbuf), const_cast<int *>(sendcnts), const_cast<int *>(displs), sendtype, 
                            recvbuf, recvcnt, recvtype, root, comm);
  }

  app_side = true;
  return mpi_ret;
}


// MPI_Reduce_scatter
int MPI_Reduce_scatter(const void *sendbuf,
                       void *recvbuf,
                       const int *recvcnts,
                       MPI_Datatype datatype,
                       MPI_Op op,
                       MPI_Comm comm)
{
  app_side = false;
  int mpi_ret = 0;
  int type_size;
  MPI_Type_size(datatype, &type_size);
  LOG_DEBUG("here inside MPI_Reduce_scatter\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {
    int myrank;
    MPI_Comm_rank(comm, &myrank);
    LOG_DEBUG("myrank=%d\n", myrank);

    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Reduce_scatter(const_cast<void *>(sendbuf), recvbuf, const_cast<int *>(recvcnts), datatype, op, comm);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cd->ptr_cd()->LogData(recvbuf, recvcnts[myrank]*type_size);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ReadData(recvbuf, recvcnts[myrank]*type_size);

      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Reduce_scatter(const_cast<void *>(sendbuf), recvbuf, const_cast<int *>(recvcnts), datatype, op, comm);
        cur_cd->ptr_cd()->LogData(recvbuf, recvcnts[myrank]*type_size);
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Reduce_scatter(const_cast<void *>(sendbuf), recvbuf, const_cast<int *>(recvcnts), datatype, op, comm);
  }

  app_side = true;
  return mpi_ret;
}


// MPI_Scan
int MPI_Scan(const void *sendbuf,
             void *recvbuf,
             int count,
             MPI_Datatype datatype,
             MPI_Op op,
             MPI_Comm comm)
{
  app_side = false;
  int mpi_ret = 0;
  int type_size;
  MPI_Type_size(datatype, &type_size);
  LOG_DEBUG("here inside MPI_Scan\n");

  CDHandle * cur_cd = GetCurrentCD();
  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
  {

    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
    {
      mpi_ret = PMPI_Scan(const_cast<void *>(sendbuf), recvbuf, count, datatype, op, comm);

      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cd->ptr_cd()->LogData(recvbuf, count*type_size);
    }
    else
    {
      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
      CommLogErrT ret = cur_cd->ptr_cd()->ReadData(recvbuf, count*type_size);

      if (ret == kCommLogCommLogModeFlip)
      {
        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
        mpi_ret = PMPI_Scan(const_cast<void *>(sendbuf), recvbuf, count, datatype, op, comm);
        cur_cd->ptr_cd()->LogData(recvbuf, count*type_size);
      }
    }
  }
  else
  {
    mpi_ret = PMPI_Scan(const_cast<void *>(sendbuf), recvbuf, count, datatype, op, comm);
  }

  app_side = true;
  return mpi_ret;
}

int MPI_Init(int *argc, char ***argv)
{
  app_side = false;
  int mpi_ret = 0;

  mpi_ret = PMPI_Init(argc, argv);

  
  app_side = true;
  return mpi_ret;
}
int MPI_Finalize(void)
{
  app_side = false;
  int mpi_ret = 0;

  mpi_ret = PMPI_Finalize();

  
  app_side = true;
  return mpi_ret;
}
//// -------------------------------------------------------------------------------------------------------
//// persistent communication requests
//// -------------------------------------------------------------------------------------------------------
//
//int MPI_Send_init(const void *buf,
//                  int count,
//                  MPI_Datatype datatype,
//                  int dest,
//                  int tag,
//                  MPI_Comm comm,
//                  MPI_Request *request)
//{
//  app_side = false;
//  int mpi_ret = 0;
//
//  LOG_DEBUG("here inside MPI_Send_init\n");
//
//  CDHandle * cur_cd = GetCurrentCD();
//  if (MASK_CDTYPE(cur_cd->ptr_cd()->GetCDType())==kRelaxed)
//  {
//
//    if (cur_cd->ptr_cd()->GetCommLogMode()==kGenerateLog)
//    {
//      mpi_ret = PMPI_Send_init(buf, count, datatype, dest, tag, comm, request);
//
//      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
//      // TODO: here
//      cur_cd->ptr_cd()->LogData(recvbuf, count*type_size);
//    }
//    else
//    {
//      LOG_DEBUG("In kReplay mode, replaying from logs...\n");
//      CommLogErrT ret = cur_cd->ptr_cd()->ReadData(recvbuf, count*type_size);
//
//      if (ret == kCommLogCommLogModeFlip)
//      {
//        LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
//        mpi_ret = PMPI_Send_init(buf, count, datatype, dest, tag, comm, request);
//        cur_cd->ptr_cd()->LogData(recvbuf, count*type_size);
//      }
//    }
//  }
//  else
//  {
//    mpi_ret = PMPI_Send_init(buf, count, datatype, dest, tag, comm, request);
//  }
//
//  app_side = true;
//  return mpi_ret;
//}


#endif

#endif
