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
#include <stdarg.h>


int rand()
{
//  printf("inside of rand()\n");
  int i;
  int size = sizeof(int);
  static int(*real_rand)(void);
  if(!real_rand)
    real_rand = (int(*)(void)) dlsym(RTLD_NEXT, "rand");
  
  if(app_side){
    app_side = false;
    bool logable  = false;
    CD* c_CD = IsLogable(&logable);
	  if(logable){
      if(c_CD->libc_log_ptr_->GetCommLogMode() == 1){
			  c_CD->libc_log_ptr_->ReadData(&i, size);
			  PRINT_LIBC("libc_log_ptr_: %p\tRE-EXECUTE MODE rand(%i) = %i\n", c_CD->libc_log_ptr_, size, i);
  		}
  		else
  		{
        i = real_rand();
   	    c_CD->libc_log_ptr_->LogData(&i, size);
        PRINT_LIBC("libc_log_ptr_: %p\t -EXECUTE MODE rand(%i) = %i\n", c_CD->libc_log_ptr_, size, i);
	    }
	  }
	  else
	  {
      i = real_rand();
	  }
    app_side = true;
  }
  else{
    i = real_rand();
  }     

  return i;
}



/*
long random()
{
//  printf("inside of random() - app_side: %i\n", app_side);
  long i;
  long size = sizeof(long);
  static long(*real_random)(void);
  if(!real_random)
    real_random = (long(*)(void)) dlsym(RTLD_NEXT, "random");
  
  if(app_side){
    bool logable  = false;
    bool reexecute = false;
    CD* c_CD = logable_execmode(&logable, &reexecute);
	  if(logable){
		  if(reexecute){
			  c_CD->libc_log_ptr_->ReadData(&i, size);
  		}
  		else
  		{
        i = real_random();
   	    c_CD->libc_log_ptr_->LogData(&i, size);
	    }
	  }
	  else
	  {
      i = real_random();
	  }
  }
  else{
    i = real_random();
  }

  return i;
}
*/
  
/*                        
int puts(const char *str)
{
  int ret;
  int size = sizeof(int);
  if(app_side){
    app_side = false;
    bool logable  = false;
    CD* c_CD = IsLogable(&logable);
	  if(logable){
      if(c_CD->libc_log_ptr_->GetCommLogMode() == 1){
			  c_CD->libc_log_ptr_->ReadData(&ret, size);
			  printf("libc_log_ptr_: %p\tRE-EXECUTE MODE printf-puts(%i) = %i\n", c_CD->libc_log_ptr_, size, ret);
        //
        printf("%s\n", str);
        //
  		}
  		else
  		{
        ret = printf("%s\n", str);
   	    c_CD->libc_log_ptr_->LogData(&ret, size);
			  printf("libc_log_ptr_: %p\t -EXECUTE MODE printf-puts(%i) = %i\n", c_CD->libc_log_ptr_, size, ret);
	    }
	  }
	  else
	  {
      ret = printf("%s\n", str);
	  }
    app_side = true;
  }
  else{
    ret = printf("%s\n", str);
  }
  return ret;
}
 
  
int printf(const char *format,...)
{
  int ret = 0;
  int size = sizeof(int);
  va_list args;
  va_start(args,format);

  if(app_side){
    app_side = false;
    bool logable  = false;
    CD* c_CD = IsLogable(&logable);
	  if(logable){
      if(c_CD->libc_log_ptr_->GetCommLogMode() == 1){
			  c_CD->libc_log_ptr_->ReadData(&ret, size);
			  printf("libc_log_ptr_: %p\tRE-EXECUTE MODE printf(%i) = %i\n", c_CD->libc_log_ptr_, size, ret);
        //
        vprintf(format,args);
        //
  		}
  		else
  		{
        ret = vprintf(format,args);
   	    c_CD->libc_log_ptr_->LogData(&ret, size);
			  printf("libc_log_ptr_: %p\t -EXECUTE MODE printf(%i) = %i\n", c_CD->libc_log_ptr_, size, ret);
	    }
	  }
	  else
	  {
      ret = vprintf(format,args);
	  }
    app_side = true;
  }
  else{
    ret = vprintf(format,args);
  }

  va_end(args);
  return ret;
} 

    */
