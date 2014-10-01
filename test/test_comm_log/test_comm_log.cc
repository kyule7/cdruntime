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
#include <iostream>
#include "../../src/cd.h"

using namespace cd;


int main()
{
  CD* root = new CD();
  CD* child1 = root->Create(); 

  //unsigned long a=2;
  //unsigned int b=1;

  //// thread 0 logs data a
  //cd_child.comm_log_.LogData(0, &a, sizeof(unsigned long));
  //// thread 1 logs data b
  //cd_child.comm_log_.LogData(1, &b, sizeof(unsigned int));

  //char * c = new char [1024];
  //for (int ii =0; ii<1024;ii++) c[ii] = ii%256;

  //// thread 0 logs data c again
  //cd_child.comm_log_.LogData(0, c, sizeof(char)*1024);
  //cd_child.comm_log_.Print();

  //printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  //unsigned long length;
  //unsigned long d;
  //cd_child.comm_log_.ReadData(0,&d, &length);
  //printf("d=%ld, length=%ld\n", d, length);

  //char * e = new char[1024];
  //cd_child.comm_log_.ReadData(0, e, &length);
  //printf("length=%ld\n", length);
  //int ii;
  //for (ii =0; ii<1024;ii++) 
  //{
  //  if (e[ii]!=c[ii])
  //  {
  //    printf("Error: e[%d]=%d, while c[%d]=%d\n",ii,e[ii],ii,c[ii]);
  //    break;
  //  }
  //}
  //if (ii==1024) printf("Success: all e equal c!\n");
  //printf("\n");

  //unsigned int f;
  //cd_child.comm_log_.ReadData(1,&f,&length);
  //printf("f=%d, length=%ld\n", f, length);

  //unsigned int g=-1;
  //cd_child.comm_log_.ReadData(0,&g,&length);
  //printf("g=%d, length=%ld\n", g, length);
  //cd_child.comm_log_.ReadData(1,&g,&length);
  //printf("g=%d, length=%ld\n", g, length);

  //cd_child.comm_log_.PackAndPushLogs(&cd_parent);

  //cd_child.comm_log_.PackAndPushLogs(&cd_parent);

  //cd_child.comm_log_.PackAndPushLogs(&cd_parent);
  //printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

  ////cd_child_2.comm_log_.Print();
  //cd_parent.comm_log_.UnpackLogsToChildCD(&cd_child_2);
  //cd_child_2.comm_log_.Print();

  //unsigned int j;
  //cd_child_2.comm_log_.ReadData(1,&j,&length);
  //printf("j=%d, length=%ld\n", j, length);

  //unsigned long k;
  //cd_child_2.comm_log_.ReadData(0,&k, &length);
  //printf("k=%ld, length=%ld\n", k, length);

  //char * h = new char[1024];
  //cd_child_2.comm_log_.ReadData(0, h, &length);
  //printf("length=%ld\n", length);
  //for (int ii =0; ii<1024;ii++) 
  //{
  //  if (h[ii]!=c[ii])
  //  {
  //    printf("Error: h[%d]=%d, while c[%d]=%d\n",ii,h[ii],ii,c[ii]);
  //    break;
  //  }
  //}
  //if (ii==1024) printf("Success: all h equal c!\n");
  //printf("\n");

  //g=-1;
  //cd_child_2.comm_log_.ReadData(0,&g,&length);
  //printf("g=%d, length=%ld\n", g, length);
  //cd_child_2.comm_log_.ReadData(1,&g,&length);
  //printf("g=%d, length=%ld\n", g, length);

  //printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

  return 0;
}
