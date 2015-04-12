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

#include "../blcr-0.9.pre0/include/libcr.h"
#include "crut_util.h"

#define SEM "MYSEM"

int main(void)
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

    setlinebuf(stdout);
    my_pid = getpid();
    filename = crut_aprintf("context.%d", my_pid);
    printf("filname: %s\n", filename);
    printf("000 Process started with pid %d\n", my_pid);
    printf("#ST_ALARM:120\n");
   
    my_id = cr_init();
    if (my_id < 0) {
       printf("XXX cr_init() failed, returning %d\n", my_id);
       exit(-1);
    } else {
       printf("001 cr_init() succeeded\n");
    }
   
    data = crut_aprintf("aaaa");
    data2 = malloc(5);
    data2[0] = 'a';
    data2[1] = 'a';
    data2[2] = 'a';
    data2[3] = 'a';
    data2[4] = '\0';

    big_array = (char *)malloc(1000000 * sizeof(char));
    big_array[0] = 5;
    big_array[100] = 100;
    printf("data: %s\n", data);
    printf("data2: %s\n", data2);
    printf("big_array[0]: %d\n", big_array[0]);
    printf("big_array[100]: %d\n", big_array[100]);
   
    printf("mem_excl result: %d\n", cr_mem_excl((char*) data, 5));
    printf("mem_excl result2: %d\n", cr_mem_excl((char*) data2, 5));
    printf("mem_excl result3: %d\n", cr_mem_excl((char*) big_array, 1000000 * sizeof(char)));
 
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
    } else if (rc == 0) {
      printf("CONTINUE!!!\n");
      printf("waiting to restart\n");
 
      getchar();
      snprintf(cmd, sizeof(cmd), "cr_restart -p %d %s", my_pid, filename);
      system(cmd);
    }
   
    printf("data: %s\n", data);
    printf("data2: %s\n", data2);
    printf("data: %p\n", data);
    printf("data2: %p\n", data2);
    printf("big_array[0]: %d\n", big_array[0]);
    printf("big_array[100]: %d\n", big_array[100]);      

    exit(0);

    return 0;
       
}
