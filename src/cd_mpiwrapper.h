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


#ifndef _CD_MPIWRAPPER_H
#define _CD_MPIWRAPPER_H

#include <stdio.h>
#include <mpi.h>

// blocking p2p communication
int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Ssend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Rsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Bsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Status *status);
int MPI_Sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, 
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, 
                MPI_Comm comm, MPI_Status *status);
int MPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest, int sendtag,
                        int source, int recvtag, MPI_Comm comm, MPI_Status *status);

// non-blocking p2p communication
int MPI_Isend(void *buf, int count, MPI_Datatype datatype, int dest, 
            int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int src, 
            int tag, MPI_Comm comm, MPI_Request *request);

int MPI_Wait(MPI_Request *request, MPI_Status *status);

// collective communication
int MPI_Barrier (MPI_Comm comm);

int MPI_Bcast (void *buffer,
               int count,
               MPI_Datatype datatype,
               int root,
               MPI_Comm comm);

int MPI_Gather(void *sendbuf,
               int sendcnt,
               MPI_Datatype sendtype,
               void *recvbuf,
               int recvcnt,
               MPI_Datatype recvtype,
               int root, 
               MPI_Comm comm);

int MPI_Gatherv(const void *sendbuf,
                int sendcnt,
                MPI_Datatype sendtype,
                void *recvbuf,
                const int *recvcnts,
                const int *displs,
                MPI_Datatype recvtype,
                int root, 
                MPI_Comm comm);

int MPI_Allgather(const void *sendbuf,
                  int sendcnt,
                  MPI_Datatype sendtype,
                  void *recvbuf,
                  int recvcnt,
                  MPI_Datatype recvtype,
                  MPI_Comm comm);

int MPI_Allgatherv(const void *sendbuf,
                   int sendcnt,
                   MPI_Datatype sendtype,
                   void *recvbuf,
                   const int *recvcnts,
                   const int *displs,
                   MPI_Datatype recvtype,
                   MPI_Comm comm);

int MPI_Reduce(const void *sendbuf,
               void *recvbuf, 
               int count,
               MPI_Datatype datatype,
               MPI_Op op, 
               int root,
               MPI_Comm comm);

int MPI_Allreduce(const void *sendbuf,
                  void *recvbuf, 
                  int count,
                  MPI_Datatype datatype,
                  MPI_Op op, 
                  MPI_Comm comm);

int MPI_Alltoall(const void *sendbuf,
                 int sendcnt,
                 MPI_Datatype sendtype,
                 void *recvbuf,
                 int recvcnt,
                 MPI_Datatype recvtype,
                 MPI_Comm comm);

int MPI_Alltoallv(const void *sendbuf,
                  const int *sendcnts,
                  const int *sdispls,
                  MPI_Datatype sendtype,
                  void *recvbuf,
                  const int *recvcnts,
                  const int *rdispls,
                  MPI_Datatype recvtype,
                  MPI_Comm comm);

int MPI_Scatter(const void *sendbuf,
                int sendcnt,
                MPI_Datatype sendtype,
                void *recvbuf,
                int recvcnt,
                MPI_Datatype recvtype,
                int root, 
                MPI_Comm comm);

int MPI_Scatterv(const void *sendbuf,
                 const int *sendcnts,
                 const int *displs,
                 MPI_Datatype sendtype,
                 void *recvbuf,
                 int recvcnt,
                 MPI_Datatype recvtype,
                 int root, 
                 MPI_Comm comm);

int MPI_Reduce_scatter(const void *sendbuf,
                       void *recvbuf,
                       const int *recvcnts,
                       MPI_Datatype datatype,
                       MPI_Op op,
                       MPI_Comm comm);

int MPI_Scan(const void *sendbuf,
             void *recvbuf,
             int count,
             MPI_Datatype datatype,
             MPI_Op op,
             MPI_Comm comm);

#endif

