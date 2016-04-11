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

#include "cd.h"
#include "cd_global.h"
#define TO_CDHandle(a) (reinterpret_cast<CDHandle*>(a))
#define TO_cdhandle(a) (reinterpret_cast<cdhandle_t*>(a))
#define TO_RegenObject(a) (reinterpret_cast<RegenObject*>(a))
#define TO_regenobject(a) (reinterpret_cast<regenobject*>(a))

cdhandle_t * cd_init(int num_tasks, int my_task, int prv_medium)
{
  return TO_cdhandle(CD_Init(num_tasks, my_task, (PrvMediumT)prv_medium));
}

void cd_finalize()
{
  CD_Finalize();
}

cdhandle_t* cd_create(uint32_t  num_children,
                           const char *name, 
                           int cd_type)
{
  return TO_cdhandle(GetCurrentCD()->Create(num_children, name, cd_type));
}

cdhandle_t* cd_create_customized(uint32_t color,
                                      uint32_t task_in_color,
                                      uint32_t  numchildren,
                                      const char *name,
                                      int cd_type)
{
  return TO_cdhandle(GetCurrentCD()->Create(color, task_in_color, numchildren, name, cd_type));
}

void cd_destroy(cdhandle* c_handle)
{
  TO_CDHandle(c_handle)->Destroy();
}

void cd_begin(cdhandle_t* c_handle)
{
  CD_Begin(TO_CDHandle(c_handle));
}

void cd_complete(cdhandle_t* c_handle)
{
  CD_Complete(TO_CDHandle(c_handle));
}

//FIXME: for now only supports this one preservation, and does not support RegenObject...
//TODO: what is this ref_name? should that be the same with my_name? 
int cd_preserve(cdhandle_t* c_handle, 
                   void *data_ptr,
                   uint64_t len,
                   uint32_t preserve_mask,
                   const char *my_name, 
                   const char *ref_name)
{
  return (int)(TO_CDHandle(c_handle)->Preserve(data_ptr, len, preserve_mask, my_name, ref_name));
}

cdhandle_t* getcurrentcd(void)
{
  return TO_cdhandle(GetCurrentCD());
}

cdhandle_t* getleafcd(void)
{
  return TO_cdhandle(GetLeafCD());
}

void cd_detect(cdhandle_t* c_handle)
{
  TO_CDHandle(c_handle)->Detect();
}

