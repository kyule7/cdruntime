/*
 * Berkeley Lab Checkpoint/Restart (BLCR) for Linux is Copyright (c)
 * 2008, The Regents of the University of California, through Lawrence
 * Berkeley National Laboratory (subject to receipt of any required
 * approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * Portions may be copyrighted by others, as may be noted in specific
 * copyright notices within specific files.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
  ../ but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: stage0001.c,v 1.8.12.1 2012/12/18 18:32:08 phargrov Exp $
 */

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#include "mpi.h"
#include "../blcr-0.9.pre0/include/libcr.h"
#include "crut_util.h"

#define SEM "MYSEM"
#define  MASTER		0

#define SIXTY_FOUR_MB 6400000
#define SIXTY_FOUR_B  64

#define MSG_SIZE SIXTY_FOUR_B

int main(int argc, char *argv[])
{
    pid_t my_pid, pid;
    char cmd[255];
    char *big_array;
    struct timespec ts;
    cr_callback_id_t cb_id;
    cr_client_id_t my_id;
    cr_checkpoint_handle_t my_handle;
    char *filename;
    char *data;
    char *data2;
    int rc, fd;
    struct stat s;
    sem_t *sem;

    int  numtasks, taskid, len;
    char hostname[MPI_MAX_PROCESSOR_NAME];
    int  partner, message, destrank, rank;
    MPI_Status status;
    MPI_Request req[3];
    MPI_Win win;
    MPI_Group comm_group, group;

    int* send_msg;
    int* recv_msg;

    int send_buf[4];
    int recv_buf[4];

    send_msg = (int*) malloc(MSG_SIZE * sizeof(int));
    recv_msg = (int*) malloc(MSG_SIZE * sizeof(int));

    printf("send_msg: %p\n", send_msg);
    printf("recv_msg: %p\n", recv_msg);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
    MPI_Get_processor_name(hostname, &len);
    setlinebuf(stdout);
    printf ("Hello from task %d on %s!\n", taskid, hostname);

    MPI_Comm_group(MPI_COMM_WORLD, &comm_group);

    if (taskid == MASTER)
        printf("MASTER: Number of MPI tasks is: %d\n",numtasks);

    /* determine partner and then send/receive with partner */
    if (taskid >= numtasks/2) {
        destrank = partner = taskid - numtasks/2;

        MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win);
        MPI_Group_incl(comm_group, 1, &destrank, &group);
        MPI_Win_start(group, 0, win);

        MPI_Send(send_msg, MSG_SIZE, MPI_INT, partner, 1, MPI_COMM_WORLD);
        MPI_Recv(recv_msg, MSG_SIZE, MPI_INT, partner, 1, MPI_COMM_WORLD, &status);

        my_pid = getpid();
        filename = crut_aprintf("context.%d", my_pid);
        printf("filename: %s\n", filename);
        printf("000 Process started with pid %d\n", my_pid);
        printf("#ST_ALARM:120\n");
       
        my_id = cr_init();
        if (my_id < 0) {
           printf("XXX cr_init() failed, returning %d\n", my_id);
           exit(-1);
        } else {
           printf("001 cr_init() succeeded\n");
        }
        
        /* Send buffer init */

        send_buf[0] = 1;
        send_buf[1] = 2;

        printf("MPI_Isend\n");
        MPI_Isend(send_msg, MSG_SIZE, MPI_INT, partner, 1, MPI_COMM_WORLD, &req[0]); // nonblocking
        printf("MPI_Isend complete\n");


        MPI_Put(&send_buf[0], 1, MPI_INT, partner, 0, 1, MPI_INT, win);
        printf("First put complete\n");

        /* Request a checkpoint of ourself - should happen NOW */
        fd = crut_checkpoint_request(&my_handle, filename);
        if (fd < 0) {
           printf("XXX crut_checkpoint_request() unexpectedly returned 0x%x\n", fd);
           exit(-1);
        }
       
        rc = stat(filename, &s);
        if (rc) {
           printf("XXX stat() unexpectedly returned %d\n", rc);
           exit(-1);
        } else {
           printf("010 stat(context.%d) correctly returned 0\n", my_pid);
        }
       
        if (s.st_size == 0) {
           printf("XXX context file unexpectedly empty\n");
           exit(-1);
        } else {
           printf("011 context.%d is non-empty\n", my_pid);
        }
       
        /* Reap the checkpoint request */
        rc = crut_checkpoint_wait(&my_handle, fd);
        if (rc < 0) {
           printf("XXX crut_checkpoint_wait() #2 unexpectedly returned 0x%x\n", rc);
           exit(-1);
        } else if (rc == 1) {
          printf("RESTART!!!!\n");

          MPI_Put(&send_buf[1], 1, MPI_INT, partner, 1, 1, MPI_INT, win);
          printf("Second put complete\n");

          MPI_Win_complete(win);

          MPI_Recv(recv_msg, MSG_SIZE, MPI_INT, partner, 1, MPI_COMM_WORLD, &status);
          printf("RECEIVE ACROSS RESTART COMPLETE!!!!\n");

          MPI_Recv(recv_msg, MSG_SIZE, MPI_INT, partner, 1, MPI_COMM_WORLD, &status);
          printf("RESTART PASSED!!!!\n");
        } else if (rc == 0) {
          printf("CONTINUE!!!\n");
          printf("waiting to restart\n");
  
          snprintf(cmd, sizeof(cmd), "cr_restart -p %d %s", my_pid, filename);
          system(cmd);
        }


    } else if (taskid < numtasks/2) {
        destrank = partner = taskid + numtasks/2;

        MPI_Win_create(recv_buf, MSG_SIZE*sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
        MPI_Group_incl(comm_group, 1, &destrank, &group);

        MPI_Recv(recv_msg, MSG_SIZE, MPI_INT, partner, 1, MPI_COMM_WORLD, &status);
        MPI_Send(send_msg, MSG_SIZE, MPI_INT, partner, 1, MPI_COMM_WORLD);

        printf("slave MPI_Isend\n");
        MPI_Isend(send_msg, MSG_SIZE, MPI_INT, partner, 1, MPI_COMM_WORLD, &req[1]); // nonblocking
        printf("slave MPI_Isend complete\n");

        printf("WAITING BEFORE RECEIVING DATA\n");

        usleep(20000000); // make sure we wait 

        printf("OUT OF WAIT\n");

        MPI_Recv(recv_msg, MSG_SIZE, MPI_INT, partner, 1, MPI_COMM_WORLD, &status);
        
        printf("RECEIVED MESSAGE\n");

        MPI_Send(send_msg, MSG_SIZE, MPI_INT, partner, 1, MPI_COMM_WORLD);

        printf("slave sent final message\n");

        MPI_Win_post(group, 0, win);
        MPI_Win_wait(win);

        printf("recv[0]: %d, recv[1]: %d\n", recv_buf[0], recv_buf[1]);

    }

    //printf("Task %d is partner with %d\n",taskid,message);

    MPI_Finalize();

    free(send_msg);
    free(recv_msg);

    exit(0);

    return 0;
       
}
