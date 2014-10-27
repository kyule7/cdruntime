#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>
#include "cd.h"
#include "cds.h" 
#include "cd_global.h"

bool app_side;
typedef void*(*Malloc)(size_t size);

using namespace cd;

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



