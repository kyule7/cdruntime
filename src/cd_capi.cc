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

#include "cd_handle.h"
#include "cd_global.h"
#include "cd_capi.h"
using namespace cd;
using namespace cd::internal;
#define TO_CDHandle(a) (reinterpret_cast<CDHandle*>(a))
#define TO_cdhandle(a) (reinterpret_cast<cd_handle_t*>(a))
#define TO_RegenObject(a) (reinterpret_cast<RegenObject*>(a))
#define TO_regenobject(a) (reinterpret_cast<regenobject*>(a))

cd_handle_t * cd_init(int num_tasks, int my_task, int prv_medium)
{
  return TO_cdhandle(CD_Init(num_tasks, my_task, (PrvMediumT)prv_medium));
}

void cd_finalize()
{
  CD_Finalize();
}


cd_handle_t* cd_create(cd_handle_t* c_handle, 
                      uint32_t  num_children,
                      const char *name, 
                      int cd_type,
                      uint32_t error_name)
{
  return TO_cdhandle(TO_CDHandle(c_handle)->Create(num_children, name, cd_type, error_name));
}

cd_handle_t* cd_create_customized(cd_handle_t *c_handle,
                                      uint32_t color,
                                      uint32_t task_in_color,
                                      uint32_t  numchildren,
                                      const char *name,
                                      int cd_type,
                                      uint32_t error_name)
{
  return TO_cdhandle(TO_CDHandle(c_handle)->Create(color, task_in_color, numchildren, name, cd_type, error_name));
}

void cd_destroy(cd_handle_t* c_handle)
{
  TO_CDHandle(c_handle)->Destroy();
}

//void cd_begin(cd_handle_t* c_handle, int collective, const char *label)
//{
//  CD_Begin(TO_CDHandle(c_handle), (bool)collective, label);
//}

int ctxt_prv_mode(cd_handle_t*c_handle)
{
  return TO_CDHandle(c_handle)->ctxt_prv_mode();
}

jmp_buf *jmp_buffer(cd_handle_t *c_handle)
{
  return TO_CDHandle(c_handle)->jmp_buffer();
}
ucontext_t *ctxt(cd_handle_t *c_handle)
{
  return TO_CDHandle(c_handle)->ctxt();
}
void commit_preserve_buff(cd_handle_t *c_handle)
{
  TO_CDHandle(c_handle)->CommitPreserveBuff();
}

void internal_begin(cd_handle_t *c_handle, int collective, const char *label)
{
  TO_CDHandle(c_handle)->InternalBegin(collective, label);
}


void cd_complete(cd_handle_t* c_handle)
{
  CD_Complete(TO_CDHandle(c_handle));
}

//FIXME: for now only supports this one preservation, and does not support RegenObject...
//TODO: what is this ref_name? should that be the same with my_name? 
int cd_preserve(cd_handle_t* c_handle, 
                   void *data_ptr,
                   uint64_t len,
                   uint32_t preserve_mask,
                   const char *my_name, 
                   const char *ref_name)
{
  return (int)(TO_CDHandle(c_handle)->Preserve(data_ptr, len, preserve_mask, my_name, ref_name));
}

cd_handle_t* getcurrentcd(void)
{
  return TO_cdhandle(GetCurrentCD());
}

cd_handle_t* getleafcd(void)
{
  return TO_cdhandle(GetLeafCD());
}

void cd_detect(cd_handle_t* c_handle)
{
  TO_CDHandle(c_handle)->Detect();
}

