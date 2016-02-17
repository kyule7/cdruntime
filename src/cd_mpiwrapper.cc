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

#include "cd_config.h"
#include "cd_features.h"

#if _MPI_VER

#ifdef comm_log

#include "cd_mpiwrapper.h"
#include "cd_path.h"
#include "cd_global.h"
#include "cd_comm_log.h"
#include "cd_def_internal.h"
using namespace cd;
clock_t cd::msg_begin_clk;
clock_t cd::msg_end_clk;
clock_t cd::msg_elapsed_time;

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
  MsgPrologue();
  int mpi_ret=0;
  LOG_DEBUG("here inside MPI_Send\n");
  LOG_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Send out of CD context...\n");
    mpi_ret = PMPI_Send(const_cast<void *>(buf), count, datatype, dest, tag, comm);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Send(buf, count, datatype, dest, tag, comm);
      break;

    case kRelaxedCDGen:
      mpi_ret = PMPI_Send(buf, count, datatype, dest, tag, comm);
      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cdh->ptr_cd()->LogData(buf, 0, dest);
      break;

    case kRelaxedCDRead:
      {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ProbeData(buf, 0);
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Send(buf, count, datatype, dest, tag, comm);
          cur_cdh->ptr_cd()->LogData(buf, 0, dest);
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
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
  MsgPrologue();
  int mpi_ret=0;
  LOG_DEBUG("here inside MPI_Ssend\n");
  LOG_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Ssend out of CD context...\n");
    mpi_ret = PMPI_Ssend(const_cast<void *>(buf), count, datatype, dest, tag, comm);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Ssend(buf, count, datatype, dest, tag, comm);
      break;

    case kRelaxedCDGen:
      mpi_ret = PMPI_Ssend(buf, count, datatype, dest, tag, comm);
      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cdh->ptr_cd()->LogData(buf, 0, dest);
      break;

    case kRelaxedCDRead:
      {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ProbeData(buf, 0);
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Ssend(buf, count, datatype, dest, tag, comm);
          cur_cdh->ptr_cd()->LogData(buf, 0, dest);
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
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
  MsgPrologue();
  int mpi_ret=0;
  LOG_DEBUG("here inside MPI_Rsend\n");
  LOG_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Rsend out of CD context...\n");
    mpi_ret = PMPI_Rsend(const_cast<void *>(buf), count, datatype, dest, tag, comm);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Rsend(buf, count, datatype, dest, tag, comm);
      break;

    case kRelaxedCDGen:
      mpi_ret = PMPI_Rsend(buf, count, datatype, dest, tag, comm);
      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cdh->ptr_cd()->LogData(buf, 0, dest);
      break;

    case kRelaxedCDRead:
      {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ProbeData(buf, 0);
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Rsend(buf, count, datatype, dest, tag, comm);
          cur_cdh->ptr_cd()->LogData(buf, 0, dest);
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
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
  MsgPrologue();
  int mpi_ret=0;
  LOG_DEBUG("here inside MPI_Bsend\n");
  LOG_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Bsend out of CD context...\n");
    mpi_ret = PMPI_Bsend(const_cast<void *>(buf), count, datatype, dest, tag, comm);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Bsend(buf, count, datatype, dest, tag, comm);
      break;

    case kRelaxedCDGen:
      mpi_ret = PMPI_Bsend(buf, count, datatype, dest, tag, comm);
      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cdh->ptr_cd()->LogData(buf, 0, dest);
      break;

    case kRelaxedCDRead:
      {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ProbeData(buf, 0);
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Bsend(buf, count, datatype, dest, tag, comm);
          cur_cdh->ptr_cd()->LogData(buf, 0, dest);
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
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
  MsgPrologue();
  int mpi_ret=0;
  int type_size;
  PMPI_Type_size(datatype, &type_size);

  LOG_DEBUG("here inside MPI_Recv\n");
  LOG_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Recv out of CD context...\n");
    mpi_ret = PMPI_Recv(const_cast<void *>(buf), count, datatype, src, tag, comm, status);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Recv(buf, count, datatype, src, tag, comm, status);
      break;

    case kRelaxedCDGen:
      mpi_ret = PMPI_Recv(buf, count, datatype, src, tag, comm, status);
      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cdh->ptr_cd()->LogData(buf, count*type_size, src);
      break;

    case kRelaxedCDRead:
      {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ReadData(buf, count*type_size);
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Recv(buf, count, datatype, src, tag, comm, status);
          cur_cdh->ptr_cd()->LogData(buf, count*type_size, src);
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
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
                 int src, 
                 int recvtag, 
                 MPI_Comm comm, 
                 MPI_Status *status)
{
  MsgPrologue();
  int mpi_ret=0;
  int type_size;
  PMPI_Type_size(recvtype, &type_size);
  LOG_DEBUG("here inside MPI_Sendrecv\n");
  LOG_DEBUG("sendbuf=%p, &sendbuf=%p\n", sendbuf, &sendbuf);
  LOG_DEBUG("recvbuf=%p, &recvbuf=%p\n", recvbuf, &recvbuf);

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Sendrecv out of CD context...\n");
    mpi_ret = PMPI_Sendrecv(const_cast<void *>(sendbuf), sendcount, sendtype, dest, sendtag,
                          recvbuf, recvcount, recvtype, src, recvtag, comm, status);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag,
                            recvbuf, recvcount, recvtype, src, recvtag, comm, status);
      break;

    case kRelaxedCDGen:
      mpi_ret = PMPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag,
                            recvbuf, recvcount, recvtype, src, recvtag, comm, status);
      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cdh->ptr_cd()->LogData(sendbuf, 0, dest);
      cur_cdh->ptr_cd()->LogData(recvbuf, recvcount*type_size, src);
      break;

    case kRelaxedCDRead:
      {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ProbeData(sendbuf, 0);
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag,
                            recvbuf, recvcount, recvtype, src, recvtag, comm, status);
          cur_cdh->ptr_cd()->LogData(sendbuf, 0, dest);
          cur_cdh->ptr_cd()->LogData(recvbuf, recvcount*type_size, src);
        }
        else if (ret == kCommLogOK)
        {
          ret = cur_cdh->ptr_cd()->ReadData(recvbuf, recvcount*type_size);
          if (ret != kCommLogOK)
          {
            ERROR_MESSAGE("Sendbuf logged while Recvbuf not!!\n");
          }
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
  return mpi_ret;
}

// send and recv a message using a single buffer
// this function is thread-safe
int MPI_Sendrecv_replace(void *buf, 
                         int count, 
                         MPI_Datatype datatype, 
                         int dest, 
                         int sendtag,
                         int src, 
                         int recvtag, 
                         MPI_Comm comm, 
                         MPI_Status *status)
{
  MsgPrologue();
  int mpi_ret=0;
  int type_size;
  PMPI_Type_size(datatype, &type_size);
  LOG_DEBUG("here inside MPI_Recv\n");
  LOG_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Sendrecv_replace out of CD context...\n");
    mpi_ret = PMPI_Sendrecv_replace(buf, count, datatype, dest, sendtag, src, recvtag, comm, status);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Sendrecv_replace(buf, count, datatype, dest, sendtag, src, recvtag, comm, status);
      break;

    case kRelaxedCDGen:
      mpi_ret = PMPI_Sendrecv_replace(buf, count, datatype, dest, sendtag, src, recvtag, comm, status);
      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cdh->ptr_cd()->LogData(buf, 0, dest);
      cur_cdh->ptr_cd()->LogData(buf, count*type_size, src);
      break;

    case kRelaxedCDRead:
      {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ProbeData(buf, 0);
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Sendrecv_replace(buf, count, datatype, dest, sendtag, src, recvtag, comm, status);
          cur_cdh->ptr_cd()->LogData(buf, 0, dest);
          cur_cdh->ptr_cd()->LogData(buf, count*type_size, src);
        }
        else if (ret == kCommLogOK)
        {
          ret = cur_cdh->ptr_cd()->ReadData(buf, count*type_size);
          if (ret != kCommLogOK)
          {
            ERROR_MESSAGE("Sendbuf logged while Recvbuf not!!\n");
          }
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
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
  MsgPrologue();
  int mpi_ret=0;
  LOG_DEBUG("here inside MPI_Isend\n");
  LOG_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh != NULL) {
    CD *cdp = cur_cdh->ptr_cd();
    MPI_Group g;
    MPI_Comm_group(comm, &g);
    switch (cdp->GetCDLoggingMode()) {
      case kStrictCD: {
//        printf("test send: strict CD\t"); cdp->CheckIntraCDMsg(dest, g);
//        mpi_ret = PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
        break;
      }
      case kRelaxedCDGen: {
//        mpi_ret = PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
        LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
  
        // KYU: Intra-CD msg check
        if(cdp->CheckIntraCDMsg(dest, g)) {
          //printf("Intra-CD message\n");
        } else { // Log message for inter-CD communication
          //printf("Inter-CD message\n");
        }
        cdp->LogData(buf, 0, dest, false, (unsigned long)request, 0, false, cdp->CheckIntraCDMsg(dest, g));
        break;
      }
      case kRelaxedCDRead: {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");

        // KYU: Intra-CD msg check
        // FIXME Both case probe data
        CommLogErrT ret = cur_cdh->ptr_cd()->ProbeData(buf, 0);

        // End of log entry before failure. Flipped to executin mode!
        if (ret == kCommLogCommLogModeFlip) {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
//          mpi_ret = PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
          cur_cdh->ptr_cd()->LogData(buf, 0, dest, false, (unsigned long)request, 0, false, cdp->CheckIntraCDMsg(dest, g));
        }
        else if (ret == kCommLogError) {
          ERROR_MESSAGE("Incomplete log entry for non-blocking communication! Needs to escalate, not implemented yet...\n");
        }
        break;
      }
      default: {
        ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
        break;
      }
    } // switch ends
  }
  else {
    LOG_DEBUG("Warning: MPI_Isend out of CD context...\n");
//    mpi_ret = PMPI_Isend(const_cast<void *>(buf), count, datatype, dest, tag, comm, request);
  }

  MsgEpilogue();
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
  MsgPrologue();
  int mpi_ret=0;
  int type_size;
  PMPI_Type_size(datatype, &type_size);
  LOG_DEBUG("here inside MPI_Irecv\n");
  LOG_DEBUG("buf=%p, &buf=%p\n", buf, &buf);

  CDHandle *cur_cdh = GetCurrentCD();
  if(cur_cdh != NULL) {
    CD *cdp = cur_cdh->ptr_cd();
    MPI_Group g;
    MPI_Comm_group(comm, &g);
    switch( cur_cdh->ptr_cd()->GetCDLoggingMode() ) {
      case kStrictCD: {
//printf("test recv: strict CD\t"); cdp->CheckIntraCDMsg(src, g);
//        mpi_ret = PMPI_Irecv(buf, count, datatype, src, tag, comm, request);
        break;
      }
      case kRelaxedCDGen: { // Execution
//        mpi_ret = PMPI_Irecv(buf, count, datatype, src, tag, comm, request);
        LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
  
        // KYU: Intra-CD msg check
        if(cdp->CheckIntraCDMsg(src, g)) { // Do not log for intra-CD msg
          //printf("Intra-CD message\n");
          // FIXME Record just event, length should be 0
          // log event to check for escalation.
          cur_cdh->ptr_cd()->LogData(buf, 0, src, false, (unsigned long)request, 1, false, true);
          
        } else { // Log message for inter-CD communication
          //printf("Inter-CD message\n");
          cur_cdh->ptr_cd()->LogData(buf, count*type_size, src, false, (unsigned long)request, 1);
        }
        break;
      } 
      case kRelaxedCDRead: {  // Reexecution
        CommLogErrT ret = kCommLogOK;
 
       // KYU: Intra-CD msg check
        if( cdp->CheckIntraCDMsg(src, g) ) { // Do not replay intra-CD msg, but reexecute it!
  
          LOG_DEBUG("In kReplay mode, but regenerate message for intra-CD messages...\n");
//          mpi_ret = PMPI_Irecv(buf, count, datatype, src, tag, comm, request);

          // Check some event (current log entry and just skip to the next one)
          // This should read the corresponding payload when it calls MPI_Waitxxx
          // FIXME Do we need this?
          ret = cur_cdh->ptr_cd()->ReadData(buf, 0);
  
        }
        else { // Do log for this to replay for inter-CD messages at error event
  
          LOG_DEBUG("In kReplay mode, replaying from logs...\n");
          ret = cur_cdh->ptr_cd()->ReadData(buf, count*type_size);
  
        }

        // End of log entry before failure. Flipped to executin mode!
        if (ret == kCommLogCommLogModeFlip) {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
//          mpi_ret = PMPI_Irecv(buf, count, datatype, src, tag, comm, request);

          // This message was inter-CD message. So, keep logging for this message, too.
          // KYU: Intra-CD msg check
          if( cdp->CheckIntraCDMsg(src, g) ) { // Do not log for intra-CD msg
            //printf("Intra-CD message\n");
            // FIXME Record just event, length should be 0
            // log event to check for escalation.
            cur_cdh->ptr_cd()->LogData(buf, 0, src, false, (unsigned long)request, 1, false, true);
            
          } 
          else { // Log message for inter-CD communication
            //printf("Inter-CD message\n");
            cur_cdh->ptr_cd()->LogData(buf, count*type_size, src, false, (unsigned long)request, 1);
          }
        }
        else if (ret == kCommLogError) {
          ERROR_MESSAGE("Incomplete log entry for non-blocking communication! Needs to escalate, not implemented yet...\n");
        }
        break;
      }
  
      default: {
        ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
        break;
      }
    } // switch ends
  }
  else {  // cur_cdh == NULL
    LOG_DEBUG("Warning: MPI_Irecv out of CD context...\n");
//    mpi_ret = PMPI_Irecv(buf, count, datatype, src, tag, comm, request);
  }
  MsgEpilogue();
  return mpi_ret;
}

// test functions 
int MPI_Test(MPI_Request *request, 
             int * flag,
             MPI_Status *status)
{
  MsgPrologue();
  int mpi_ret = 0;

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh != NULL) {
    switch ( cur_cdh->ptr_cd()->GetCDLoggingMode() ) {
      case kStrictCD: {
        mpi_ret = PMPI_Test(request, flag, status);
        // delete incomplete entries...
        if (*flag == 1)
        {
          cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)request);
        }
        break;
      }
      case kRelaxedCDGen: {
        mpi_ret = PMPI_Test(request, flag, status);
        LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
        if (*flag == 1)
        {
          LOG_DEBUG("Operation complete, log flag and data...\n");
          cur_cdh->ptr_cd()->LogData(flag, sizeof(int), 0);
          cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)request);
        }
        else
        {
          //LOG_DEBUG("Operation not complete, log flag...\n");
          cur_cdh->ptr_cd()->LogData(flag, sizeof(int), 0, true, 0, false, true);
        }
        break;
      }
      case kRelaxedCDRead: {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ReadData(flag, sizeof(int));
        if (ret == kCommLogOK) {
          if (*flag == 1) {
            cur_cdh->ptr_cd()->ProbeData(request, 0);
          }
        }
        else if (ret == kCommLogCommLogModeFlip) {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Test(request, flag, status);
          if (*flag == 1) {
            LOG_DEBUG("Operation complete, log flag and data...\n");
            cur_cdh->ptr_cd()->LogData(flag, sizeof(int), 0);
            cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)request);
          }
          else {
            LOG_DEBUG("Operation not complete, log flag...\n");
            cur_cdh->ptr_cd()->LogData(flag, sizeof(int), 0, true, 0, false, true);
          }
        }
        else if (ret == kCommLogError) {
          ERROR_MESSAGE("Incomplete log entry for non-blocking communication! Needs to escalate, not implemented yet...\n");
        }
        break;
      }
      default: {
        ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
        break;
      }
    } // switch ends
  }
  else {
    //LOG_DEBUG("Warning: MPI_Test out of CD context...\n");
    mpi_ret = PMPI_Test(request, flag, status);
  }

  MsgEpilogue();
  return mpi_ret;
}


int MPI_Testall(int count,
                MPI_Request array_of_requests[],
                int *flag,
                MPI_Status array_of_statuses[])
{
  MsgPrologue();
  int mpi_ret = 0;
  LOG_DEBUG("here inside MPI_Testall\n");

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh != NULL) {
    switch (cur_cdh->ptr_cd()->GetCDLoggingMode()) {
      case kStrictCD: {
        mpi_ret = PMPI_Testall(count, array_of_requests, flag, array_of_statuses);
        // delete incomplete entries...
        if (*flag == 1)
        {
          for (int ii=0;ii<count;ii++)
          {
            cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[ii]);
          }
        }
        break;
      }
      case kRelaxedCDGen: {
        mpi_ret = PMPI_Testall(count, array_of_requests, flag, array_of_statuses);
        LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
        if (*flag == 1)
        {
          LOG_DEBUG("Operation complete, log flag and data...\n");
          cur_cdh->ptr_cd()->LogData(flag, sizeof(int), 0);
          for (int ii=0;ii<count;ii++)
          {
            LOG_DEBUG("Log data with count(%d) and index(%d)...\n",count,ii);
            cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[ii]);
          }
          LOG_DEBUG("Passed log flag and data...\n");
        }
        else
        {
          cur_cdh->ptr_cd()->LogData(flag, sizeof(int), 0, true, 0, false, true);
        }
        break;
      }
      case kRelaxedCDRead: {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ReadData(flag, sizeof(int));
        if (ret == kCommLogOK)
        {
          if (*flag == 1)
          {
            for (int ii=0;ii<count;ii++)
            {
              cur_cdh->ptr_cd()->ProbeData(&array_of_requests[ii], 0);
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
            cur_cdh->ptr_cd()->LogData(flag, sizeof(int), 0);
            for (int ii=0;ii<count;ii++)
            {
              cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[ii]);
            }
          }
          else
          {
            cur_cdh->ptr_cd()->LogData(flag, sizeof(int), 0, true, 0, false, true);
          }
        }
        else if (ret == kCommLogError)
        {
          ERROR_MESSAGE("Incomplete log entry for non-blocking communication! Needs to escalate, not implemented yet...\n");
        }
        break;
      } 
      default: {
        ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
        break;
      }
    } // switch ends
  }
  else {
    //LOG_DEBUG("Warning: MPI_Testall out of CD context...\n");
    mpi_ret = PMPI_Testall(count, array_of_requests, flag, array_of_statuses);
  }
  MsgEpilogue();
  return mpi_ret;
}

int MPI_Testany(int count,
                MPI_Request array_of_requests[],
                int *index,
                int *flag,
                MPI_Status *status)
{
  MsgPrologue();
  int mpi_ret = 0;
  LOG_DEBUG("here inside MPI_Testany\n");

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    //LOG_DEBUG("Warning: MPI_Testany out of CD context...\n");
    mpi_ret = PMPI_Testany(count, array_of_requests, index, flag, status);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Testany(count, array_of_requests, index, flag, status);
      // delete incomplete entries...
      if (*flag == 1)
      {
        cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[*index]);
      }
      break;

    case kRelaxedCDGen:
      mpi_ret = PMPI_Testany(count, array_of_requests, index, flag, status);
      if (*flag == 1)
      {
        LOG_DEBUG("Operation complete, log flag and data...\n");
        cur_cdh->ptr_cd()->LogData(flag, sizeof(int), 0);
        cur_cdh->ptr_cd()->LogData(index, sizeof(int), 0);
        cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[*index]);
      }
      else
      {
        cur_cdh->ptr_cd()->LogData(flag, sizeof(int), 0, true, 0, false, true);
      }
      break;

    case kRelaxedCDRead:
      {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ReadData(flag, sizeof(int));
        if (ret == kCommLogOK)
        {
          if (*flag == 1)
          {
            cur_cdh->ptr_cd()->ReadData(index, sizeof(int));
            cur_cdh->ptr_cd()->ProbeData(&array_of_requests[*index], 0);
          }
        }
        else if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Testany(count, array_of_requests, index, flag, status);
          if (*flag == 1)
          {
            LOG_DEBUG("Operation complete, log flag and data...\n");
            cur_cdh->ptr_cd()->LogData(flag, sizeof(int), 0);
            cur_cdh->ptr_cd()->LogData(index, sizeof(int), 0);
            cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[*index]);
          }
          else
          {
            cur_cdh->ptr_cd()->LogData(flag, sizeof(int), 0, true, 0, false, true);
          }
        }
        else if (ret == kCommLogError)
        {
          ERROR_MESSAGE("Incomplete log entry for non-blocking communication! Needs to escalate, not implemented yet...\n");
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
  return mpi_ret;
}

int MPI_Testsome(int incount,
                 MPI_Request array_of_requests[],
                 int *outcount,
                 int array_of_indices[],
                 MPI_Status array_of_statuses[])
{
  MsgPrologue();
  int mpi_ret = 0;
  LOG_DEBUG("here inside MPI_Testsome\n");

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    //LOG_DEBUG("Warning: MPI_Testsome out of CD context...\n");
    mpi_ret = PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
      // delete incomplete entries...
      if (*outcount > 0)
      {
        for (int ii=0;ii<*outcount;ii++)
        {
          cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[array_of_indices[ii]]);
        }
      }
      break;

    case kRelaxedCDGen:
      mpi_ret = PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      if (*outcount > 0)
      {
        LOG_DEBUG("Operation complete, log outcount and data...\n");
        cur_cdh->ptr_cd()->LogData(outcount, sizeof(int), 0);
        for (int ii=0;ii<*outcount;ii++)
        {
          cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[array_of_indices[ii]]);
        }
      }
      else
      {
        cur_cdh->ptr_cd()->LogData(outcount, sizeof(int), 0, true, 0, false, true);
      }
      break;

    case kRelaxedCDRead:
      {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ReadData(outcount, sizeof(int));
        if (ret == kCommLogOK)
        {
          if (*outcount > 0)
          {
            for (int ii=0;ii<*outcount;ii++)
            {
              cur_cdh->ptr_cd()->ProbeData(&array_of_requests[array_of_indices[ii]], 0);
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
            cur_cdh->ptr_cd()->LogData(outcount, sizeof(int), 0);
            for (int ii=0;ii<*outcount;ii++)
            {
              cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[array_of_indices[ii]]);
            }
          }
          else
          {
            cur_cdh->ptr_cd()->LogData(outcount, sizeof(int), 0, true, 0, false, true);
          }
        }
        else if (ret == kCommLogError)
        {
          ERROR_MESSAGE("Incomplete log entry for non-blocking communication! Needs to escalate, not implemented yet...\n");
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
  return mpi_ret;
}

// wait functions
// Kyushick modified
// LogData : mostly for blocking. if length is 0, it recording msg event itself, msg payload 
// ReadData : replay
// ProbeData : read data with length 0 (barrier. it is for reading msg event itself, not msg payload)
// ProbeAndLogData : it is only for MPI_Wait and MPI_Test.
int MPI_Wait(MPI_Request *request, 
             MPI_Status *status)
{
  MsgPrologue();
  int mpi_ret = 0;
  LOG_DEBUG("here inside MPI_Wait\n");

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh != NULL) {
    switch ( cur_cdh->ptr_cd()->GetCDLoggingMode() ) {
      case kStrictCD: {
//printf("test wait : strict CD\t"); //cdp->CheckIntraCDMsg(dest, g);
//        mpi_ret = PMPI_Wait(request, status);
        // delete incomplete entries...
        cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)request);
        break;
      }
      case kRelaxedCDGen: {
// FIXME        mpi_ret = PMPI_Wait(request, status);
        LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");

//        if( cur_cdh->CheckIntraCDMsg(dest, g) ) {
//          printf("Intra-CD message\n");
//        } 
        cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)request);
        break;
      } 
      case kRelaxedCDRead: {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ProbeData(request,0);
        if (ret == kCommLogCommLogModeFlip) {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          LOG_DEBUG("Should not come here because error happens between Isend/Irecv and WaitXXX...\n");
//FIXME          mpi_ret = PMPI_Wait(request, status);
          cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)request);
        }
        else if (ret == kCommLogError) {
          ERROR_MESSAGE("Incomplete log entry for non-blocking communication! Needs to escalate, not implemented yet...\n");
        }
        break;
      }
      default: {
        ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
        break;
      }
    } // switch ends
  }
  else {
    LOG_DEBUG("Warning: MPI_Wait out of CD context...\n");
//    mpi_ret = PMPI_Wait(request, status);
  }
  MsgEpilogue();
  return mpi_ret;
}

// wait functions 
// Kyushick modified
int MPI_Waitall(int count, MPI_Request array_of_requests[], 
                MPI_Status array_of_statuses[])
{
  MsgPrologue();
  int mpi_ret = 0;
  int ii=0;
  LOG_DEBUG("here inside MPI_Waitall\n");

  CDHandle *cur_cdh = GetCurrentCD();
  if (cur_cdh != NULL) {
    switch( cur_cdh->ptr_cd()->GetCDLoggingMode() ) {
      case kStrictCD: {
//printf("test waitall: strict CD\t"); //cdp->CheckIntraCDMsg(dest, g);
        mpi_ret = PMPI_Waitall(count, array_of_requests, array_of_statuses);
        // delete incomplete entries...
        for (ii=0;ii<count;ii++) {
          cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[ii]);
        }
        break;
      }
      case kRelaxedCDGen: {  // execution
        mpi_ret = PMPI_Waitall(count, array_of_requests, array_of_statuses);
        LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
  
        // KYU: Intra-CD msg check
//        printf("Intra-CD message? %d\n", cur_cdh->CheckIntraCDMsg(dest, g));
  
        for (ii=0;ii<count;ii++) { // probe incomplete, log data and log event
          cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[ii]);
        }
    
        break;
      }
      case kRelaxedCDRead: { // Reexecution. Replay logs!
        // For MPI_Wait in reexecution path, this performs any useful things,
        // MPI_Wait just check events.
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = kCommLogOK;

        for (ii=0;ii<count;ii++) {
          // intra-CD msg, it reads event
          ret = cur_cdh->ptr_cd()->ProbeData(&array_of_requests[ii], 0); // read event
          if (ret == kCommLogCommLogModeFlip) {
            if (ii != 0) {
//              ERROR_MESSAGE("Partially instrumented MPI_Waitall, may cause incorrect re-execution!!\n"); FIXME
            }
            break;
          }
        }

        if (ret == kCommLogCommLogModeFlip) { // flipped to execution
          LOG_DEBUG("Isn't it weird to reach here because error before wait should escalate it\n");
//          assert(0); FIXME
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          LOG_DEBUG("Should not come here because error happens between Isend/Irecv and WaitXXX...\n");
          mpi_ret = PMPI_Waitall(count, array_of_requests, array_of_statuses);
          for (ii=0;ii<count;ii++) {
            cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[ii]);
          }
        }
        else if (ret == kCommLogError) {
          ERROR_MESSAGE("Incomplete log entry for non-blocking communication! Needs to escalate, not implemented yet...\n");
        }
        break;
      }
      default: {
        ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
        break;
      }
    } // switch ends
  }
  else {
    LOG_DEBUG("Warning: MPI_Waitall out of CD context...\n");
    mpi_ret = PMPI_Waitall(count, array_of_requests, array_of_statuses);
  }

  MsgEpilogue();
  return mpi_ret;
}

// wait functions 
int MPI_Waitany(int count, MPI_Request *array_of_requests, 
                int *index, MPI_Status *status)
{
  MsgPrologue();
  int mpi_ret = 0;
  LOG_DEBUG("here inside MPI_Waitany\n");

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Waitany out of CD context...\n");
    mpi_ret = PMPI_Waitany(count, array_of_requests, index, status);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Waitany(count, array_of_requests, index, status);
      // delete incomplete entries...
      cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[*index]);
      break;

    case kRelaxedCDGen:
      mpi_ret = PMPI_Waitany(count, array_of_requests, index, status);
      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cdh->ptr_cd()->LogData(index, sizeof(int), 0);
      cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[*index]);
      break;

    case kRelaxedCDRead:
      {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ReadData(index, sizeof(int));
        if (ret == kCommLogOK)
        {
          ret = cur_cdh->ptr_cd()->ProbeData(&array_of_requests[*index],0);
        }
        else if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          LOG_DEBUG("Should not come here because error happens between Isend/Irecv and WaitXXX...\n");
          mpi_ret = PMPI_Waitany(count, array_of_requests, index, status);
          cur_cdh->ptr_cd()->LogData(index, sizeof(int), 0);
          cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[*index]);
        }
        else if (ret == kCommLogError)
        {
          ERROR_MESSAGE("Incomplete log entry for non-blocking communication! Needs to escalate, not implemented yet...\n");
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
  return mpi_ret;
}

int MPI_Waitsome(int incount,
                 MPI_Request array_of_requests[],
                 int *outcount,
                 int array_of_indices[],
                 MPI_Status array_of_statuses[])
{
  MsgPrologue();
  int mpi_ret = 0;
  int ii=0;
  LOG_DEBUG("here inside MPI_Waitsome\n");

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Waitsome out of CD context...\n");
    mpi_ret = PMPI_Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
      // delete incomplete entries...
      for (ii=0;ii<*outcount;ii++)
      {
        cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[array_of_indices[ii]]);
      }
      break;

    case kRelaxedCDGen:
      mpi_ret = PMPI_Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cdh->ptr_cd()->LogData(outcount, sizeof(int), 0);
      cur_cdh->ptr_cd()->LogData(array_of_indices, *outcount*sizeof(int), 0);
      for (ii=0;ii<*outcount;ii++)
      {
        cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[array_of_indices[ii]]);
      }
      break;

    case kRelaxedCDRead:
      {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ReadData(outcount, sizeof(int));
        if (ret == kCommLogOK)
        {
          cur_cdh->ptr_cd()->ReadData(array_of_indices, *outcount*sizeof(int));
          for (ii=0;ii<*outcount;ii++)
          {
            cur_cdh->ptr_cd()->ReadData(&array_of_requests[array_of_indices[ii]], 0);
          }
        }
        else if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          LOG_DEBUG("Should not come here because error happens between Isend/Irecv and WaitXXX...\n");
          mpi_ret = PMPI_Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
          cur_cdh->ptr_cd()->LogData(outcount, sizeof(int), 0);
          cur_cdh->ptr_cd()->LogData(array_of_indices, *outcount*sizeof(int), 0);
          for (ii=0;ii<*outcount;ii++)
          {
            cur_cdh->ptr_cd()->ProbeAndLogData((unsigned long)&array_of_requests[array_of_indices[ii]]);
          }
        }
        else if (ret == kCommLogError)
        {
          ERROR_MESSAGE("Incomplete log entry for non-blocking communication! Needs to escalate, not implemented yet...\n");
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
  return mpi_ret;
}

// -------------------------------------------------------------------------------------------------------
// collective communication
// -------------------------------------------------------------------------------------------------------

// MPI_Barrier
int MPI_Barrier (MPI_Comm comm)
{
  MsgPrologue();
  int mpi_ret=0;

  int myrank;
  PMPI_Comm_rank(comm, &myrank);
  LOG_DEBUG("(%d)here inside MPI_Barrier.\n", myrank);

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Barrier out of CD context...\n");
    mpi_ret = PMPI_Barrier(comm);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Barrier(comm);
      break;

    case kRelaxedCDGen:
      mpi_ret = PMPI_Barrier(comm);
      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cdh->ptr_cd()->LogData(&comm, 0, myrank);
      break;

    case kRelaxedCDRead:
      {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ProbeData(&comm, 0);
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Barrier(comm);
          cur_cdh->ptr_cd()->LogData(&comm, 0, myrank);
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
  return mpi_ret;
}

// MPI_Bcast
int MPI_Bcast (void *buffer,
               int count,
               MPI_Datatype datatype,
               int root,
               MPI_Comm comm)
{
  MsgPrologue();
  int mpi_ret=0;
  int type_size;
  PMPI_Type_size(datatype, &type_size);
  LOG_DEBUG("here inside MPI_Bcast\n");

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Bcast out of CD context...\n");
    mpi_ret = PMPI_Bcast(buffer, count, datatype, root, comm);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Bcast(buffer, count, datatype, root, comm);
      break;

    case kRelaxedCDGen:
      mpi_ret = PMPI_Bcast(buffer, count, datatype, root, comm);
      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cdh->ptr_cd()->LogData(buffer, count*type_size, root);
      break;

    case kRelaxedCDRead:
      {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ReadData(buffer, count*type_size);
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Bcast(buffer, count, datatype, root, comm);
          cur_cdh->ptr_cd()->LogData(buffer, count*type_size, root);
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
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
  MsgPrologue();
  int mpi_ret = 0;
  int type_size;
  PMPI_Type_size(recvtype, &type_size);
  LOG_DEBUG("here inside MPI_Gather\n");

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Gather out of CD context...\n");
    mpi_ret = PMPI_Gather(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
    MsgEpilogue();
    return mpi_ret;
  }

  int myrank, size;
  PMPI_Comm_rank(comm, &myrank);
  MPI_Comm_size(comm, &size);
  LOG_DEBUG("myrank=%d, size=%d\n", myrank, size);
  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Gather(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
      break;

    case kRelaxedCDGen:
      mpi_ret = PMPI_Gather(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      if (myrank == root){
        cur_cdh->ptr_cd()->LogData(recvbuf, size*recvcnt*type_size, root);
      }
      else {
        cur_cdh->ptr_cd()->LogData(sendbuf, 0, root);
      }
      break;

    case kRelaxedCDRead:
      {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret;
        if (myrank == root){
          ret = cur_cdh->ptr_cd()->ReadData(recvbuf, size*recvcnt*type_size);
        }
        else{
          ret = cur_cdh->ptr_cd()->ProbeData(sendbuf, 0);
        }
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Gather(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
          if (myrank == root) {
            cur_cdh->ptr_cd()->LogData(recvbuf, size*recvcnt*type_size, root);
          }
          else {
            cur_cdh->ptr_cd()->LogData(sendbuf, 0, root);
          }
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
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
  MsgPrologue();
  int mpi_ret = 0;
  int type_size;
  PMPI_Type_size(recvtype, &type_size);
  LOG_DEBUG("here inside MPI_Gatherv\n");

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Gatherv out of CD context...\n");
    mpi_ret = PMPI_Gatherv(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, const_cast<int *>(recvcnts), const_cast<int *>(displs), recvtype, root, comm);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Gatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcnts, displs, recvtype, root, comm);
      break;

    case kRelaxedCDGen:
      {
        int myrank, size;
        PMPI_Comm_rank(comm, &myrank);
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
        // issue mpi comm operation
        mpi_ret = PMPI_Gatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcnts, displs, recvtype, root, comm);
        LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
        if (myrank == root){
          cur_cdh->ptr_cd()->LogData(recvbuf, totalcnts*type_size, root);
        }
        else {
          cur_cdh->ptr_cd()->LogData(sendbuf, 0, root);
        }
      }
      break;

    case kRelaxedCDRead:
      {
        int myrank, size;
        PMPI_Comm_rank(comm, &myrank);
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
        // issue mpi comm operation
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret;
        if (myrank == root){
          ret = cur_cdh->ptr_cd()->ReadData(recvbuf, totalcnts*type_size);
        }
        else{
          ret = cur_cdh->ptr_cd()->ProbeData(sendbuf, 0);
        }
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Gatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcnts, displs, recvtype, root, comm);
          if (myrank == root) {
            cur_cdh->ptr_cd()->LogData(recvbuf, totalcnts*type_size, root);
          }
          else {
            cur_cdh->ptr_cd()->LogData(sendbuf, 0, root);
          }
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
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
  MsgPrologue();
  int mpi_ret = 0;
  int myrank;
  PMPI_Comm_rank(comm, &myrank);
  int type_size;
  PMPI_Type_size(recvtype, &type_size);
  LOG_DEBUG("here inside MPI_Allgather\n");

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Allgather out of CD context...\n");
    mpi_ret = PMPI_Allgather(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, recvcnt, recvtype, comm);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Allgather(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, comm);
      break;

    case kRelaxedCDGen:
      {
        int size;
        MPI_Comm_size(comm, &size);
        LOG_DEBUG("size=%d\n", size);
        mpi_ret = PMPI_Allgather(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, comm);
        LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
        cur_cdh->ptr_cd()->LogData(recvbuf, size*recvcnt*type_size, myrank);
      }
      break;

    case kRelaxedCDRead:
      {
        int size;
        MPI_Comm_size(comm, &size);
        LOG_DEBUG("size=%d\n", size);
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ReadData(recvbuf, size*recvcnt*sizeof(recvtype));
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Allgather(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, comm);
          cur_cdh->ptr_cd()->LogData(recvbuf, size*recvcnt*type_size, myrank);
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
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
  MsgPrologue();
  int mpi_ret = 0;
  int myrank;
  PMPI_Comm_rank(comm, &myrank);
  int type_size;
  PMPI_Type_size(recvtype, &type_size);
  LOG_DEBUG("here inside MPI_Allgatherv\n");

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Allgatherv out of CD context...\n");
    mpi_ret = PMPI_Allgatherv(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, const_cast<int *>(recvcnts), const_cast<int *>(displs), recvtype, comm);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Allgatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcnts, displs, recvtype, comm);
      break;

    case kRelaxedCDGen:
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
        mpi_ret = PMPI_Allgatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcnts, displs, recvtype, comm);
        LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
        cur_cdh->ptr_cd()->LogData(recvbuf, totalcnts*type_size, myrank);
      }
      break;

    case kRelaxedCDRead:
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
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ReadData(recvbuf, totalcnts*type_size);
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Allgatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcnts, displs, recvtype, comm);
          cur_cdh->ptr_cd()->LogData(recvbuf, totalcnts*type_size, myrank);
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
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
  MsgPrologue();
  int mpi_ret = 0;
  int type_size;
  PMPI_Type_size(datatype, &type_size);
  LOG_DEBUG("here inside MPI_Reduce\n");

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Reduce out of CD context...\n");
    mpi_ret = PMPI_Reduce(const_cast<void *>(sendbuf), recvbuf, count, datatype, op, root, comm);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
      break;

    case kRelaxedCDGen:
      {
        int myrank, size;
        PMPI_Comm_rank(comm, &myrank);
        MPI_Comm_size(comm, &size);
        LOG_DEBUG("myrank=%d, size=%d\n", myrank, size);
        mpi_ret = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);

        LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
        if (myrank == root){
          cur_cdh->ptr_cd()->LogData(recvbuf, count*type_size, root);
        }
        else {
          cur_cdh->ptr_cd()->LogData(sendbuf, 0, root);
        }
      }
      break;

    case kRelaxedCDRead:
      {
        int myrank, size;
        PMPI_Comm_rank(comm, &myrank);
        MPI_Comm_size(comm, &size);
        LOG_DEBUG("myrank=%d, size=%d\n", myrank, size);
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret;
        if (myrank == root){
          ret = cur_cdh->ptr_cd()->ReadData(recvbuf, count*type_size);
        }
        else{
          ret = cur_cdh->ptr_cd()->ProbeData(sendbuf, 0);
        }
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
          if (myrank == root) {
            cur_cdh->ptr_cd()->LogData(recvbuf, count*type_size, root);
          }
          else {
            cur_cdh->ptr_cd()->LogData(sendbuf, 0, root);
          }
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
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
  MsgPrologue();
  int mpi_ret = 0;
  int myrank;
  PMPI_Comm_rank(comm, &myrank);
  int type_size;
  PMPI_Type_size(datatype, &type_size);
  LOG_DEBUG("here inside MPI_Allreduce\n");

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Allreduce out of CD context...\n");
    mpi_ret = PMPI_Allreduce(const_cast<void *>(sendbuf), recvbuf, count, datatype, op, comm);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
      break;

    case kRelaxedCDGen:
      mpi_ret = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cdh->ptr_cd()->LogData(recvbuf, count*type_size, myrank);
      break;

    case kRelaxedCDRead:
      {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ReadData(recvbuf, count*type_size);
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm);
          cur_cdh->ptr_cd()->LogData(recvbuf, count*type_size, myrank);
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
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
  MsgPrologue();
  int mpi_ret = 0;
  int myrank;
  PMPI_Comm_rank(comm, &myrank);
  int type_size;
  PMPI_Type_size(recvtype, &type_size);
  LOG_DEBUG("here inside MPI_Alltoall\n");

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Alltoall out of CD context...\n");
    mpi_ret = PMPI_Alltoall(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, recvcnt, recvtype, comm);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Alltoall(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, comm);
      break;

    case kRelaxedCDGen:
      {
        int size;
        MPI_Comm_size(comm, &size);
        LOG_DEBUG("size=%d\n", size);
        mpi_ret = PMPI_Alltoall(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, comm);
        LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
        cur_cdh->ptr_cd()->LogData(recvbuf, size*recvcnt*type_size, myrank);
      }
      break;

    case kRelaxedCDRead:
      {
        int size;
        MPI_Comm_size(comm, &size);
        LOG_DEBUG("size=%d\n", size);
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ReadData(recvbuf, size*recvcnt*type_size);
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Alltoall(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, comm);
          cur_cdh->ptr_cd()->LogData(recvbuf, size*recvcnt*type_size, myrank);
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
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
  MsgPrologue();
  int mpi_ret = 0;
  int myrank;
  PMPI_Comm_rank(comm, &myrank);
  int type_size;
  PMPI_Type_size(recvtype, &type_size);
  LOG_DEBUG("here inside MPI_Alltoallv\n");

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Alltoallv out of CD context...\n");
    mpi_ret = PMPI_Alltoallv(const_cast<void *>(sendbuf), const_cast<int *>(sendcnts), const_cast<int *>(sdispls), sendtype, 
                             recvbuf, const_cast<int *>(recvcnts), const_cast<int *>(rdispls), recvtype, comm);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Alltoallv(sendbuf, sendcnts, sdispls, sendtype, 
                               recvbuf, recvcnts, rdispls, recvtype, comm);
      break;

    case kRelaxedCDGen:
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
        mpi_ret = PMPI_Alltoallv(sendbuf, sendcnts, sdispls, sendtype, 
                                 recvbuf, recvcnts, rdispls, recvtype, comm);
        LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
        cur_cdh->ptr_cd()->LogData(recvbuf, totalcnts*type_size, myrank);
      }
      break;

    case kRelaxedCDRead:
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
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ReadData(recvbuf, totalcnts*type_size);
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Alltoallv(sendbuf, sendcnts, sdispls, sendtype, 
                                   recvbuf, recvcnts, rdispls, recvtype, comm);

          cur_cdh->ptr_cd()->LogData(recvbuf, totalcnts*type_size, myrank);
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
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
  MsgPrologue();
  int mpi_ret = 0;
  int type_size;
  PMPI_Type_size(recvtype, &type_size);
  LOG_DEBUG("here inside MPI_Scatter\n");

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Scatter out of CD context...\n");
    mpi_ret = PMPI_Scatter(const_cast<void *>(sendbuf), sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Scatter(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
      break;

    case kRelaxedCDGen:
      mpi_ret = PMPI_Scatter(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cdh->ptr_cd()->LogData(recvbuf, recvcnt*type_size, root);
      break;

    case kRelaxedCDRead:
      {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ReadData(recvbuf, recvcnt*type_size);
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Scatter(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
          cur_cdh->ptr_cd()->LogData(recvbuf, recvcnt*type_size, root);
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
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
  MsgPrologue();
  int mpi_ret = 0;
  int type_size;
  PMPI_Type_size(recvtype, &type_size);
  LOG_DEBUG("here inside MPI_Scatterv\n");

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Scatterv out of CD context...\n");
    mpi_ret = PMPI_Scatterv(const_cast<void *>(sendbuf), const_cast<int *>(sendcnts), const_cast<int *>(displs), sendtype, 
                            recvbuf, recvcnt, recvtype, root, comm);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Scatterv(sendbuf, sendcnts, displs, sendtype, 
                              recvbuf, recvcnt, recvtype, root, comm);
      break;

    case kRelaxedCDGen:
      mpi_ret = PMPI_Scatterv(sendbuf, sendcnts, displs, sendtype, 
                              recvbuf, recvcnt, recvtype, root, comm);
      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cdh->ptr_cd()->LogData(recvbuf, recvcnt*type_size, root);
      break;

    case kRelaxedCDRead:
      {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ReadData(recvbuf, recvcnt*type_size);
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Scatterv(sendbuf, sendcnts, displs, sendtype, 
                                  recvbuf, recvcnt, recvtype, root, comm);
          cur_cdh->ptr_cd()->LogData(recvbuf, recvcnt*type_size, root);
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
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
  MsgPrologue();
  int mpi_ret = 0;
  int type_size;
  PMPI_Type_size(datatype, &type_size);
  LOG_DEBUG("here inside MPI_Reduce_scatter\n");

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Reduce_scatter out of CD context...\n");
    mpi_ret = PMPI_Reduce_scatter(const_cast<void *>(sendbuf), recvbuf, const_cast<int *>(recvcnts), datatype, op, comm);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Reduce_scatter(sendbuf, recvbuf, recvcnts, datatype, op, comm);
      break;

    case kRelaxedCDGen:
      {
        int myrank;
        PMPI_Comm_rank(comm, &myrank);
        LOG_DEBUG("myrank=%d\n", myrank);
        mpi_ret = PMPI_Reduce_scatter(sendbuf, recvbuf, recvcnts, datatype, op, comm);
        LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
        cur_cdh->ptr_cd()->LogData(recvbuf, recvcnts[myrank]*type_size, myrank);
      }
      break;

    case kRelaxedCDRead:
      {
        int myrank;
        PMPI_Comm_rank(comm, &myrank);
        LOG_DEBUG("myrank=%d\n", myrank);
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ReadData(recvbuf, recvcnts[myrank]*type_size);
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Reduce_scatter(sendbuf, recvbuf, recvcnts, datatype, op, comm);
          cur_cdh->ptr_cd()->LogData(recvbuf, recvcnts[myrank]*type_size, myrank);
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
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
  MsgPrologue();
  int mpi_ret = 0;
  int type_size;
  PMPI_Type_size(datatype, &type_size);
  int myrank;
  PMPI_Comm_rank(comm, &myrank);
  LOG_DEBUG("here inside MPI_Scan\n");

  CDHandle * cur_cdh = GetCurrentCD();
  if (cur_cdh == NULL)
  {
    LOG_DEBUG("Warning: MPI_Scan out of CD context...\n");
    mpi_ret = PMPI_Scan(const_cast<void *>(sendbuf), recvbuf, count, datatype, op, comm);
    MsgEpilogue();
    return mpi_ret;
  }

  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
  {
    case kStrictCD:
      mpi_ret = PMPI_Scan(sendbuf, recvbuf, count, datatype, op, comm);
      break;

    case kRelaxedCDGen:
      mpi_ret = PMPI_Scan(sendbuf, recvbuf, count, datatype, op, comm);
      LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
      cur_cdh->ptr_cd()->LogData(recvbuf, count*type_size, myrank);
      break;

    case kRelaxedCDRead:
      {
        LOG_DEBUG("In kReplay mode, replaying from logs...\n");
        CommLogErrT ret = cur_cdh->ptr_cd()->ReadData(recvbuf, count*type_size);
        if (ret == kCommLogCommLogModeFlip)
        {
          LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
          mpi_ret = PMPI_Scan(sendbuf, recvbuf, count, datatype, op, comm);
          cur_cdh->ptr_cd()->LogData(recvbuf, count*type_size, myrank);
        }
      }
      break;

    default:
      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
      break;
  }

  MsgEpilogue();
  return mpi_ret;
}

int MPI_Init(int *argc, char ***argv)
{
  MsgPrologue();
  int mpi_ret = 0;

  mpi_ret = PMPI_Init(argc, argv);

  
  MsgEpilogue();
  return mpi_ret;
}
int MPI_Finalize(void)
{
  MsgPrologue();
  int mpi_ret = 0;

  mpi_ret = PMPI_Finalize();

  
  MsgEpilogue();
  return mpi_ret;
}
int MPI_Group_translate_ranks(MPI_Group group1, int n, const int ranks1[],
                              MPI_Group group2, int ranks2[]) {
  assert(group2);
  return PMPI_Group_translate_ranks(group1, n, ranks1,
                                    group2, ranks2);
}
unsigned epoch_num = 0;
int MPI_Win_fence(int assert, MPI_Win win)
{
  CD_DEBUG("[%s %u] called %s at %u (#reexec: %d)\n", __func__, epoch_num++,GetCurrentCD()->ptr_cd()->name(), GetCurrentCD()->ptr_cd()->level(), GetCurrentCD()->ptr_cd()->num_reexec());
#if CD_DEBUG_DEST == 1
  auto &reexec_map = GetCurrentCD()->ptr_cd()->num_exec_map();
  for(auto it=reexec_map.begin(); it!=reexec_map.end(); ++it) {
    CD_DEBUG("%s : %d/%d\n", it->first.c_str(), it->second.second,it->second.first);
  }

  fflush(cdout);
#endif
  int ret = PMPI_Win_fence(assert, win);
  return ret;
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
//  MsgPrologue();
//  int mpi_ret = 0;
//
//  LOG_DEBUG("here inside MPI_Send_init\n");
//
//  CDHandle * cur_cdh = GetCurrentCD();
//  if (cur_cdh == NULL)
//  {
//    LOG_DEBUG("Warning: MPI_Send_init out of CD context...\n");
//    mpi_ret = PMPI_Send_init(buf, count, datatype, dest, tag, comm, request);
//    MsgEpilogue();
//    return mpi_ret;
//  }
//  switch (cur_cdh->ptr_cd()->GetCDLoggingMode())
//  {
//    case kStrictCD:
//      break;
//
//    case kRelaxedCDGen:
//      break;
//
//    case kRelaxedCDRead:
//      break;
//
//    default:
//      ERROR_MESSAGE("Wrong number for enum CDLoggingMode (%d)!\n", cur_cdh->ptr_cd()->GetCDLoggingMode());
//      break;
//  }
//
//  //if (cur_cdh->ptr_cd()->GetCDType()==kRelaxed)
//  //{
//
//  //  if (cur_cdh->ptr_cd()->GetCommLogMode()==kGenerateLog)
//  //  {
//  //    mpi_ret = PMPI_Send_init(buf, count, datatype, dest, tag, comm, request);
//
//  //    LOG_DEBUG("In kGenerateLog mode, generating new logs...\n");
//  //    // TODO: following function call is wrong, should add parameter for thread info
//  //    cur_cdh->ptr_cd()->LogData(recvbuf, count*type_size);
//  //  }
//  //  else
//  //  {
//  //    LOG_DEBUG("In kReplay mode, replaying from logs...\n");
//  //    CommLogErrT ret = cur_cdh->ptr_cd()->ReadData(recvbuf, count*type_size);
//  //    if (ret == kCommLogCommLogModeFlip)
//  //    {
//  //      LOG_DEBUG("Reached end of logs, and begin to generate logs...\n");
//  //      mpi_ret = PMPI_Send_init(buf, count, datatype, dest, tag, comm, request);
//  //      cur_cdh->ptr_cd()->LogData(recvbuf, count*type_size);
//  //    }
//  //  }
//  //}
//  //else
//  //{
//  //  mpi_ret = PMPI_Send_init(buf, count, datatype, dest, tag, comm, request);
//  //}
//
//  MsgEpilogue();
//  return mpi_ret;
//}

#endif

#endif // ifdef comm_log
