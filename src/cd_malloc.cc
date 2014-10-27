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

#include "cd_malloc.h"

bool app_side;

CD* logable_execmode(bool *logable_, bool *execmode)
{
  printf("logable_execmode\n");      
  CDHandle* current = CDPath::GetCurrentCD();
	CD* c_CD;
	if(current==NULL){
		printf("\tbefore root CD\n");
	}
	else
	{
		c_CD = current->ptr_cd();
		if(c_CD == NULL)
		{
			printf("\tCD object associated with CD handle\n");
		}
		else
		{
			if(c_CD->libc_log_ptr_ == NULL)
			{
				printf("\tno libc_log in current CD\n");
			}
			else 
			{
				printf("\tnow we have libc_log object\n");
				*logable_ = true;
				if(c_CD->cd_exec_mode_)
					*execmode = true;
			}
		}
	}
  return c_CD;
}

//GONG: Is free() required?
void free(void *p)
{
  static void(*real_free)(void*);
  if(!real_free)
  {
    real_free = (void(*)(void *)) dlsym(RTLD_NEXT, "free");
  }
  real_free(p);
}

void* real_malloc_(size_t size)
{
//	printf("inside of real_malloc_()\n");
	Malloc real_malloc = (Malloc)dlsym(RTLD_NEXT, "malloc");
	return real_malloc(size);
}


void* malloc(size_t size)
{
	void* p;
//  printf("app side?? %d\n", app_side);
///GONG: libc logging takes care of only "application-side" malloc, malloc (both explicitly and internally) called in application.
if(app_side){
	bool logable  = false;
	bool reexecute = false;
  CD* c_CD = logable_execmode(&logable, &reexecute);
//  printf("logable? %i reexecute? %i\n", logable, reexecute);
	if(logable){
//  CDHandle* current = CDPath::GetCurrentCD();
//	CD* c_CD = current->ptr_cd();
	//GONG: Determine whether we call real_malloc w/ logging return value or get return value stored in logs
		if(reexecute){
			c_CD->libc_log_ptr_->ReadData(&p, size);
			printf("RE-EXECUTE MODE malloc(%ld) = %p\n", size, p);
		}
		else
		{
			p = real_malloc_(size);
			printf("EXECUTE MODE malloc(%ld) = %p\n", size, p);
		  c_CD->libc_log_ptr_->LogData(&p, size);
		}
	}
	else
	{
		p = real_malloc_(size);
		printf("NORMAL malloc(%ld) = %p\n", size, p);	
	}
}
else{
		p = real_malloc_(size);
//		printf("CD runtime malloc(%ld) = %p\n", size, p);	
}	
	return p;
} 


static void* temp_calloc(size_t num, size_t size)
{
  printf("empty calloc is called\n");
  return NULL;
}

extern "C" void *calloc(size_t num, size_t size)
{
  static void * (*real_calloc)(size_t,size_t) = 0;
  void *p;    
  if(!real_calloc)
  {
    real_calloc = temp_calloc;
    real_calloc = (void*(*)(size_t, size_t)) dlsym(RTLD_NEXT, "calloc");
  }
  if(app_side){
    bool logable  = false;
    bool reexecute = false;
    CD* c_CD = logable_execmode(&logable, &reexecute);
	  if(logable){
	  //GONG: Determine whether we call real_calloc w/ logging return value or get return value stored in logs
		  if(reexecute){
			  c_CD->libc_log_ptr_->ReadData(&p, size);
			  printf("RE-EXECUTE MODE calloc(%ld) = %p\n", size, p);
  		}
  		else
  		{
        p = real_calloc(num,size);
   		  printf("EXECUTE MODE calloc(%ld) = %p\n", size, p);
   	    c_CD->libc_log_ptr_->LogData(&p, size);
	    }
	  }
	  else
	  {
      p = real_calloc(num,size);
		  printf("NORMAL calloc(%ld) = %p\n", size, p);	
	  }
  }
  else{
    //NEVER invoke functions that internally invoke calloc again (e.g., printf()) in order to aviod infinite calloc loop
    p = real_calloc(num,size);
  }	        
  return p;
}

                  
void *valloc(size_t size)
{
  void *p;    
  static void * (*real_valloc)(size_t);
  if(!real_valloc)
  {
    real_valloc = (void*(*)(size_t)) dlsym(RTLD_NEXT, "valloc");
  }
  if(app_side){
    bool logable  = false;
    bool reexecute = false;
    CD* c_CD = logable_execmode(&logable, &reexecute);
	  if(logable){
	  //GONG: Determine whether we call real_valloc w/ logging return value or get return value stored in logs
		  if(reexecute){
			  c_CD->libc_log_ptr_->ReadData(&p, size);
			  printf("RE-EXECUTE MODE valloc(%ld) = %p\n", size, p);
  		}
  		else
  		{
        p = real_valloc(size);
   		  printf("EXECUTE MODE valloc(%ld) = %p\n", size, p);
   	    c_CD->libc_log_ptr_->LogData(&p, size);
	    }
	  }
	  else
	  {
      p = real_valloc(size);
		  printf("NORMAL valloc(%ld) = %p\n", size, p);	
	  }
  }
  else{
    p = real_valloc(size);
		printf("CD runtime side valloc(%ld) = %p\n", size, p);	
  }	          
  return p;
}


void *realloc(void* ptr, size_t size)
{
  void *p;    
  static void * (*real_realloc)(void*,size_t);
  if(!real_realloc)
  {
    real_realloc = (void*(*)(void*,size_t)) dlsym(RTLD_NEXT, "realloc");
  }
  if(app_side){
    bool logable  = false;
    bool reexecute = false;
    CD* c_CD = logable_execmode(&logable, &reexecute);
	  if(logable){
	  //GONG: Determine whether we call real_realloc w/ logging return value or get return value stored in logs
		  if(reexecute){
			  c_CD->libc_log_ptr_->ReadData(&p, size);
			  printf("RE-EXECUTE MODE realloc(%ld) = %p\n", size, p);
  		}
  		else
  		{
        p = real_realloc(ptr,size);
   		  printf("EXECUTE MODE realloc(%ld) = %p\n", size, p);
   	    c_CD->libc_log_ptr_->LogData(&p, size);
	    }
	  }
	  else
	  {
      p = real_realloc(ptr,size);
		  printf("NORMAL realloc(%ld) = %p\n", size, p);	
	  }
  }
  else{
    p = real_realloc(ptr,size);
		printf("CD runtime side realloc(%ld) = %p\n", size, p);	
  }	          
  return p;
}



