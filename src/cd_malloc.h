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

#ifndef _CD_MALLOC_H 
#define _CD_MALLOC_H

#ifdef libc_log

#include <stdint.h>
#include <stdio.h>
#include <dlfcn.h>
#include "cd.h"
#include "cd_global.h"
#include "cd_def_internal.h"

//bool app_side;
typedef void*(*Malloc)(size_t size);

namespace cd {

class RuntimeLogger {
	friend class CD;
	friend class HeadCD;
	public:
    static CD   *IsLogable(bool *logable_);
    static int   cd_fprintf(FILE *str, const char *format, va_list &args);
    static int   fclose(FILE *fp);
    static FILE *fopen(const char *file, const char *mode);
    static void *valloc(size_t size);
    static void *calloc(size_t num, size_t size);
    static void *malloc(size_t size);
    static void  free(void *p);
};


//CD *IsLogable(bool *logable_);
struct IncompleteLogEntry NewLogEntry(void* p, size_t size, bool FreeInvoked);


}

#endif

#endif
