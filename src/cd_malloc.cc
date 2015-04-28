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

#ifdef libc_log
#include <stdarg.h>
#include "cd_malloc.h"
#include "cd_global.h"

//bool app_side;

using namespace cd;
using namespace cd::logging;
using namespace cd::internal;

IncompleteLogEntry RuntimeLogger::NewLogEntry(void* p, size_t size, bool FreeInvoked, unsigned int level, unsigned long index)
{
  IncompleteLogEntry tmp_log_entry;
  tmp_log_entry.addr_ = (unsigned long) 0;
  tmp_log_entry.length_ = (unsigned long) size;
  tmp_log_entry.flag_ = (unsigned long) index;
  tmp_log_entry.complete_ = false;
  tmp_log_entry.isrecv_ = FreeInvoked;
  tmp_log_entry.p_ = p;
  tmp_log_entry.pushed_ = false;
  tmp_log_entry.level_ = level;
  return tmp_log_entry;
}

CD *RuntimeLogger::IsLogable(bool *logable_)
{
//  LIBC_DEBUG("logable_execmode\n");      
  CDHandle* current = GetCurrentCD();
//  if(current != NULL) {
//  std::cout << current->node_id() << " / " << current->GetCDName() <<  std::endl;
//  } else {
//    std::cout << "CD is null" << std::endl;
//  }
//  printf("current cd : %s / %s\n", current->ptr_cd_);

	CD* c_CD;
	if(current==NULL){
//		LIBC_DEBUG("\tbefore root CD\n");
	}
	else
	{
		c_CD = current->ptr_cd();
		if(c_CD == NULL)
		{
//			LIBC_DEBUG("\tCD object associated with CD handle\n");
		}
		else
		{
			if(c_CD->libc_log_ptr() == NULL && c_CD->GetBegin_())
			{
//				LIBC_DEBUG("\tno libc_log in current CD\n");
			}
			else 
			{
//				LIBC_DEBUG("\tnow we have libc_log object\n");
				*logable_ = true;
			}
		}
	}
  return c_CD;
}


//CD* RuntimeLogger::IsLogable(bool *logable_)
//{
////  LIBC_DEBUG("logable_execmode\n");      
//  CDHandle* current = GetCurrentCD();
//	CD* c_CD;
//	if(current==NULL){
////		LIBC_DEBUG("\tbefore root CD\n");
//	}
//	else
//	{
//		c_CD = current->ptr_cd();
//		if(c_CD == NULL)
//		{
////			LIBC_DEBUG("\tCD object associated with CD handle\n");
//		}
//		else
//		{
//			if(c_CD->libc_log_ptr_ == NULL && c_CD->GetBegin_())
//			{
////				LIBC_DEBUG("\tno libc_log in current CD\n");
//			}
//			else 
//			{
////				LIBC_DEBUG("\tnow we have libc_log object\n");
//				*logable_ = true;
//			}
//		}
//	}
//  return c_CD;
//}





//GONG: Is free() required?
void free(void *p)
{
  RuntimeLogger::free(p);
}

void RuntimeLogger::free(void *p)
{

  //check first whether CD-runtime side or application side (logged)
  if(app_side){
    app_side = false;
	  bool logable  = false;
    CD* c_CD = IsLogable(&logable);
	  if(logable){
      CD_DEBUG("libc_free: FREE is invoked %p\n", p);
      std::vector<IncompleteLogEntry>::iterator it;
      for (it=c_CD->mem_alloc_log_.begin(); it!=c_CD->mem_alloc_log_.end(); it++)
      {
        //address matched!
        if(it->p_ == p) 
        { 
          CD_DEBUG("libc_free: - complete? %p\n", p);
//          it->complete_ = true;
          app_side = true;
          return;
        }
      }
      //search & parent's log
      bool found = c_CD->PushedMemLogSearch(p, c_CD);
      if(found) 
      {
        CD_DEBUG("libc_free: found %p from parent's log will NOT free it \n", p);
        app_side = true;
        return;
      }
	  CD_DEBUG("inside of real_malloc_()\n");
    }

    app_side = true;
  }
  //set real_free first
  static void(*real_free)(void*);
  if(!real_free)
  {
    real_free = (void(*)(void *)) dlsym(RTLD_NEXT, "free");
  }
  real_free(p);
}

void* real_malloc_(size_t size)
{
	Malloc real_malloc = (Malloc)dlsym(RTLD_NEXT, "malloc");
	return real_malloc(size);
}


void* malloc(size_t size)
{
  return RuntimeLogger::malloc(size);
}

void* RuntimeLogger::malloc(size_t size)
{
	void* p;
///GONG: libc logging takes care of only "application-side" malloc, malloc (both explicitly and internally) called in application.

  if(app_side){
    app_side = false;
	  bool logable  = false;
    CD* c_CD = IsLogable(&logable);
  	if(logable){
  //  CDHandle* current = GetCurrentCD();
  //	CD* c_CD = current->ptr_cd();
  	//GONG: Determine whether we call real_malloc w/ logging return value or get return value stored in logs
      if(c_CD->libc_log_ptr_->GetCommLogMode() == 1 ){
        //SZ
  //			c_CD->libc_log_ptr_->ReadData(&p, size);
              CD_DEBUG("here! replay logs for malloc\n");
        if(c_CD->mem_alloc_log_.size()==0){
          CD_DEBUG("RE-EXECUTION MODE, but no entries in malloc log! => get log from parent @ level %u\n", c_CD->level());
          p = c_CD->MemAllocSearch(c_CD, c_CD->level(), c_CD->replay_pos_mem_alloc_log);
          //just increase index only for search purpose
          c_CD->replay_pos_mem_alloc_log++;
        }       
        else
        {
          p = (void*) c_CD->mem_alloc_log_.at(c_CD->replay_pos_mem_alloc_log).p_;
          if(c_CD->replay_pos_mem_alloc_log == c_CD->mem_alloc_log_.size()){
            //simply reset
            c_CD->replay_pos_mem_alloc_log = 0;
          }
          else
          {
            c_CD->replay_pos_mem_alloc_log++;
          }
        }
  			CD_DEBUG("libc_log_ptr_: %p\tRE-EXECUTE MODE malloc(%ld) = %p\n", c_CD->libc_log_ptr_, size, p);
  		}
  		else
  		{
  			p = real_malloc_(size);
        IncompleteLogEntry log_entry = NewLogEntry(p, size, true, c_CD->level(), c_CD->cur_pos_mem_alloc_log);//c_CD->mem_alloc_log_.size());
        c_CD->cur_pos_mem_alloc_log++;
        c_CD->mem_alloc_log_.push_back(log_entry);
  			CD_DEBUG("libc_log_ptr_: %p\t - EXECUTE MODE malloc(%ld) = %p @level %i  how many mem alloc logged?%lu\n", c_CD->libc_log_ptr_, size, p, c_CD->level(), c_CD->mem_alloc_log_.size());
  		}
  	}
  	else
  	{
  		p = real_malloc_(size);
  		LIBC_DEBUG("NORMAL malloc(%ld) = %p\n", size, p);	
  	}
    app_side = true;
  }
  else
  {
    p = real_malloc_(size);
  //		LIBC_DEBUG("CD runtime malloc(%ld) = %p\n", size, p);	
  }	
	return p;
} 





static void* temp_calloc(size_t num, size_t size)
{
  LIBC_DEBUG("empty calloc is called\n");
  return NULL;
}

extern "C" void *calloc(size_t num, size_t size)
{
  return RuntimeLogger::calloc(num, size);
}

void *RuntimeLogger::calloc(size_t num, size_t size)
{
  static void * (*real_calloc)(size_t,size_t) = 0;
  void *p;    
  if(!real_calloc)
  {
    real_calloc = temp_calloc;
    real_calloc = (void*(*)(size_t, size_t)) dlsym(RTLD_NEXT, "calloc");
  }



  if(app_side){
    app_side = false;
    bool logable  = false;
    CD* c_CD = IsLogable(&logable);
	  if(logable){
	  //GONG: Determine whether we call real_calloc w/ logging return value or get return value stored in logs
      if(c_CD->libc_log_ptr_->GetCommLogMode() == 1){
			//  c_CD->libc_log_ptr_->ReadData(&p, size);
      if(c_CD->mem_alloc_log_.size()==0){
        LIBC_DEBUG("RE-EXECUTION MODE, but no entries in malloc log! => get log from parent\n");
        //p = c_CD->MemAllocSearch(c_CD);
        //p = c_CD->MemAllocSearch(c_CD, c_CD->level());
          p = c_CD->MemAllocSearch(c_CD, c_CD->level(), c_CD->cur_pos_mem_alloc_log);
          if(c_CD->cur_pos_mem_alloc_log == c_CD->mem_alloc_log_.size())
            c_CD->cur_pos_mem_alloc_log = 0;
          else
            c_CD->cur_pos_mem_alloc_log++;
          
      }       
      else
      {
        p = (void*) c_CD->mem_alloc_log_.at(c_CD->cur_pos_mem_alloc_log).p_;
        if(c_CD->cur_pos_mem_alloc_log == c_CD->mem_alloc_log_.size()){
          //simply reset
          c_CD->cur_pos_mem_alloc_log = 0;
        }
        else
        {
          c_CD->cur_pos_mem_alloc_log++;
        }
      }
			  LIBC_DEBUG("libc_log_ptr_: %p\tRE-EXECUTE MODE calloc(%ld) = %p\n", c_CD->libc_log_ptr_, size, p);
  		}
  		else
  		{
        p = real_calloc(num,size);
        IncompleteLogEntry log_entry = NewLogEntry(p, size, true, c_CD->level(), c_CD->mem_alloc_log_.size());
        c_CD->mem_alloc_log_.push_back(log_entry);
			  LIBC_DEBUG("libc_log_ptr_: %p\tEXECUTE MODE - calloc(%ld) = %p\n", c_CD->libc_log_ptr_, size, p);
   	  //  c_CD->libc_log_ptr_->LogData(&p, size);
	    }
	  }
	  else
	  {
      p = real_calloc(num,size);
		  LIBC_DEBUG("NORMAL calloc(%ld) = %p\n", size, p);	
	  }
    
    app_side = true;
  }
  else{
    //NEVER invoke functions that internally invoke calloc again (e.g., LIBC_DEBUG()) in order to aviod infinite calloc loop
    p = real_calloc(num,size);
  }	        
  return p;
}

                  
void *valloc(size_t size)
{
  return RuntimeLogger::valloc(size);
}
void *RuntimeLogger::valloc(size_t size)
{
  void *p;    
  static void * (*real_valloc)(size_t);
  if(!real_valloc)
  {
    real_valloc = (void*(*)(size_t)) dlsym(RTLD_NEXT, "valloc");
  }
  if(app_side){
    app_side = false;
    bool logable  = false;
    CD* c_CD = IsLogable(&logable);
	  if(logable){
	  //GONG: Determine whether we call real_valloc w/ logging return value or get return value stored in logs
      if(c_CD->libc_log_ptr_->GetCommLogMode() == 1){
			//  c_CD->libc_log_ptr_->ReadData(&p, size);
      if(c_CD->mem_alloc_log_.size()==0){
        LIBC_DEBUG("RE-EXECUTION MODE, but no entries in malloc log! => get log from parent\n");
        //p = c_CD->MemAllocSearch(c_CD);
        //p = c_CD->MemAllocSearch(c_CD, c_CD->level());
          p = c_CD->MemAllocSearch(c_CD, c_CD->level(), c_CD->cur_pos_mem_alloc_log);
          if(c_CD->cur_pos_mem_alloc_log == c_CD->mem_alloc_log_.size())
            c_CD->cur_pos_mem_alloc_log = 0;
          else
            c_CD->cur_pos_mem_alloc_log++;
      }       
      else
      {
        p = (void*) c_CD->mem_alloc_log_.at(c_CD->cur_pos_mem_alloc_log).p_;
        if(c_CD->cur_pos_mem_alloc_log == c_CD->mem_alloc_log_.size()){
          //simply reset
          c_CD->cur_pos_mem_alloc_log = 0;
        }
        else
        {
          c_CD->cur_pos_mem_alloc_log++;
        }
      }
			  LIBC_DEBUG("libc_log_ptr_: %p\tRE-EXECUTE MODE valloc(%ld) = %p\n", c_CD->libc_log_ptr_, size, p);
  		}
  		else
  		{
        p = real_valloc(size);
        IncompleteLogEntry log_entry = NewLogEntry(p, size, true, c_CD->level(), c_CD->mem_alloc_log_.size());
        c_CD->mem_alloc_log_.push_back(log_entry);
			  LIBC_DEBUG("libc_log_ptr_: %p\tEXECUTE MODE valloc(%ld) = %p\n", c_CD->libc_log_ptr_, size, p);
  // 	    c_CD->libc_log_ptr_->LogData(&p, size);
        //IncompleteLogEntry log_entry = NewLogEntry(p, size, true);
        //c_CD->mem_alloc_log_.push_back(log_entry);
	    }
	  }
	  else
	  {
      p = real_valloc(size);
		  LIBC_DEBUG("NORMAL valloc(%ld) = %p\n", size, p);	
	  }
    app_side = true;
  }
  else{
    p = real_valloc(size);
//		LIBC_DEBUG("CD runtime side valloc(%ld) = %p\n", size, p);	
  }	          
  return p;
}

FILE* fopen(const char *file, const char *mode)
{
  return RuntimeLogger::fopen(file, mode);
}
FILE* RuntimeLogger::fopen(const char *file, const char *mode)
{
  FILE* ret;
  int size = sizeof(FILE*);
  static FILE* (*real_fopen)(const char*, const char*);
  if(!real_fopen)
    real_fopen = (FILE* (*)(const char*, const char*)) dlsym(RTLD_NEXT, "fopen");

  if(app_side){
    app_side = false;
    bool logable  = false;
    CD* c_CD = IsLogable(&logable);
	  if(logable){
      if(c_CD->libc_log_ptr_->GetCommLogMode() == 1){
        if(c_CD->mem_alloc_log_.size()==0){
          LIBC_DEBUG("RE-EXECUTION MODE (fopen), but no entries in malloc log! => get log from parent\n");
          //replace (close and re-open) FILE pointer
          FILE* tmp_ret = (*real_fopen)(file,mode);
          //ret = (FILE*) c_CD->MemAllocSearch(c_CD, c_CD->level(), tmp_ret);
          ret = (FILE*)c_CD->MemAllocSearch(c_CD, c_CD->level(), c_CD->cur_pos_mem_alloc_log, tmp_ret);
          if(c_CD->cur_pos_mem_alloc_log == c_CD->mem_alloc_log_.size())
            c_CD->cur_pos_mem_alloc_log = 0;
          else
            c_CD->cur_pos_mem_alloc_log++;
          fclose(ret);
          ret = tmp_ret;
        }       
        else
        {
          ret = (FILE*) c_CD->mem_alloc_log_.at(c_CD->cur_pos_mem_alloc_log).p_;
          if(c_CD->cur_pos_mem_alloc_log == c_CD->mem_alloc_log_.size()){
            c_CD->cur_pos_mem_alloc_log = 0;
          }
          else
          {
            c_CD->cur_pos_mem_alloc_log++;
          }
        }
			  //c_CD->libc_log_ptr_->ReadData(&ret, size);
			  LIBC_DEBUG("libc_log_ptr_: %p\tRE-EXECUTE MODE fopen(%i) = %p\n", c_CD->libc_log_ptr_, size, ret);
  		}
  		else
  		{
        ret = (*real_fopen)(file,mode);
   	   // c_CD->libc_log_ptr_->LogData(&ret, size);
			  LIBC_DEBUG("libc_log_ptr_: %p\t -EXECUTE MODE fopen(%i) = %p\n", c_CD->libc_log_ptr_, size, ret);
        IncompleteLogEntry log_entry = NewLogEntry(ret, size, false, c_CD->level(), c_CD->mem_alloc_log_.size());
        c_CD->mem_alloc_log_.push_back(log_entry);
        //IncompleteLogEntry log_entry = NewLogEntry(ret, size, false);
        //c_CD->mem_alloc_log_.push_back(log_entry);
	    }
	  }
	  else
	  {
      ret = (*real_fopen)(file,mode);
      LIBC_DEBUG("NORMAL fopen(%i) = %p\n", size, ret);
	  }
    app_side = true;
  }
  else
  {
    ret = (*real_fopen)(file,mode);
  }     
  return ret;
}


int fclose(FILE *fp)
{
  return RuntimeLogger::fclose(fp);
}
int RuntimeLogger::fclose(FILE *fp)
{
  //set real_free first
  static int(*real_fclose)(FILE*);
  if(!real_fclose)
  {
    real_fclose = (int(*)(FILE *)) dlsym(RTLD_NEXT, "fclose");
  }
  //check first whether CD-runtime side or application side (logged)
  if(app_side){
	  bool logable  = false;
    CD* c_CD = IsLogable(&logable);
	  if(logable){
//      LIBC_DEBUG("FREE invoked %p\n", p);
      std::vector<IncompleteLogEntry>::iterator it;
      for (it=c_CD->mem_alloc_log_.begin(); it!=c_CD->mem_alloc_log_.end(); it++)
      {
        //address matched!
        if(it->p_ == fp) 
        { 
//          LIBC_DEBUG("FREE- complete? %p\n", p);
          it->complete_ = true;
          return 0;
        }
      }
      //search & parent's log
      bool found = c_CD->PushedMemLogSearch(fp, c_CD);
      if(found) 
      {
//        LIBC_DEBUG("found %p from parent's log will NOT free it \n", p);
        return 0;
      }
    }
  }
  int ret = real_fclose(fp);
  return ret;
}

int fprintf(FILE *str, const char *format,...)
{
  va_list args;
  va_start(args,format);
  return RuntimeLogger::cd_fprintf(str, format, args);
}
int RuntimeLogger::cd_fprintf(FILE *str, const char *format, va_list &args)
{
  int ret = 0;
//  va_list args;
//  va_start(args,format);

  if(app_side){
    app_side = false;
    CD* c_CD = GetCurrentCD()->ptr_cd();
    if(c_CD!=NULL){
  int size = sizeof(int);
//    bool logable  = false;
//    CD* c_CD = IsLogable(&logable);
//	  if(logable){
      if(c_CD->cd_exec_mode_ != 0){
//			  c_CD->libc_log_ptr_->ReadData(&ret, size);
			  LIBC_DEBUG("libc_log_ptr_: %p\tRE-EXECUTE MODE fprintf(%d) = %d\n", c_CD->libc_log_ptr_, size, ret);
        //
//        vprintf(format,args);
        //
  		}
  		else
  		{
        ret = vfprintf(str,format,args);
//   	    c_CD->libc_log_ptr_->LogData(&ret, size);
			  LIBC_DEBUG("libc_log_ptr_: %p\t -EXECUTE MODE fprintf(%d) = %d\n", c_CD->libc_log_ptr_, size, ret);
	    }
    }
    else
    {
      ERROR_MESSAGE("out of CD runtime side!");
    }
    app_side = true;
  }
  else{
    LIBC_DEBUG("out of CD runtime side!");
    ret = vfprintf(str,format,args);
  }

  va_end(args);
  return ret;
} 
/*
void *realloc(void* ptr, size_t size)
{
  void *p;    
  static void * (*real_realloc)(void*,size_t);
  if(!real_realloc)
  {
    real_realloc = (void*(*)(void*,size_t)) dlsym(RTLD_NEXT, "realloc");
  }
  if(app_side){
    app_side = false;
    bool logable  = false;
    CD* c_CD = IsLogable(&logable);
	  if(logable){
	  //GONG: Determine whether we call real_realloc w/ logging return value or get return value stored in logs
      if(c_CD->libc_log_ptr_->GetCommLogMode() == 1){
//			  c_CD->libc_log_ptr_->ReadData(&p, size);
      p = (void*) c_CD->mem_alloc_log_.at(c_CD->cur_pos_mem_alloc_log).p_;
      if(c_CD->cur_pos_mem_alloc_log == c_CD->mem_alloc_log_.size()){
        //simply reset
        c_CD->cur_pos_mem_alloc_log = 0;
      }
      else
      {
        c_CD->cur_pos_mem_alloc_log++;
      }
			  LIBC_DEBUG("libc_log_ptr_: %p\tRE-EXECUTE MODE realloc(%ld) = %p\n", c_CD->libc_log_ptr_, size, p);
  		}
  		else
  		{
        p = real_realloc(ptr,size);
			  LIBC_DEBUG("libc_log_ptr_: %p\tEXECUTE MODE realloc(%ld) = %p\n", c_CD->libc_log_ptr_, size, p);
   	  //  c_CD->libc_log_ptr_->LogData(&p, size);
        struct IncompleteLogEntry log_entry = NewLogEntry(p, size);
        c_CD->mem_alloc_log_.push_back(log_entry);
	    }
	  }
	  else
	  {
      p = real_realloc(ptr,size);
		  LIBC_DEBUG("NORMAL realloc(%ld) = %p\n", size, p);	
	  }
    app_side = true;
  }
  else{
    p = real_realloc(ptr,size);
//		LIBC_DEBUG("CD runtime side realloc(%ld) = %p\n", size, p);	
  }	          
  return p;
}
*/




//void* malloc(size_t size)
//{
//  void* p = real_malloc_(size);
////  printf("malloc(%ld) = %p\n", size, p);
//  return p;
//}

#endif
