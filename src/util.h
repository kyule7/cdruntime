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
#ifndef _UTIL_H 
#define _UTIL_H
#include "cd_global.h"
#include <sys/types.h>
#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include "cd_id.h"
// system specific or machine specific utils resides here.

class cd::Util {
  public:
    static uint64_t GetCurrentTaskID()
    {
      // STUB

      return 0;
    }

    static pid_t GetCurrentProcessID()
    {
      //FIXME maybe it is the same task id 
      return getpid();	
    }

    static uint64_t GetCurrentDomainID()
    {
      //STUB
      return 0;
    }

    static uint64_t GetCurrentNodeID()
    {

      //STUB 
      return 0;

    }

    static uint64_t GenerateCDObjectID() 
    {
      // STUB
      // The policy for Generating CDID could be different . 
      // But this should be unique
      //FIXME for multithreaded version this is not safe 
      // Assume we call this function one time, so we will have atomically increasing counter and this will be local to a process. 
      // Within a process it will just use one counter. Check whether this is enough or not.
      static uint64_t object_id=0;
      return object_id++;

    }

    static const char * GetCDFilesPath()
    {

      //FIXME This should return appropriate path depending on configuration 
      // and perhaps each node may have different path to store CD related files
      return "./"; 

    }

    // This generates a unique file name that will be used for preservation. 
    // Per CD must be different, so we can refer to CDID and 
    // also HEX address of the pointer we are preserving. 
    // -> This might not be a good thing when we recover actually the stack content can be different... 
    // is it? or is it not?  let's assume it does...
////    static void GetUniqueCDFileName(CDID &cd_id, void *addr, char *filename) 
////    { 
//////      const char *base_file_path = GetCDFilesPath(cd_id); 
////      const char* base_file_path = GetCDFilesPath();
//////#ifdef _WORK 
////      sprintf(filename, "%s%lld.%lld.%lld.%lld.%lld.cd", base_file_path, 
////                                              (long long)cd_id.domain_id_, 
////                                              (long long)cd_id.node_id_.task_, 
////                                              (long long)cd_id.level_, 
////                                              (long long)cd_id.object_id_, 
////                                              (long long)cd_id.sequential_id_);
//////#else
//////      sprintf(filename, "%s%lld.%lld.%lld.%lld.%lld.%p.cd", base_file_path, 
//////                                                 (long long)cd_id.domain_id_, 
//////                                                 (long long)cd_id.node_id_.task_, 
//////                                                 (long long)cd_id.level_, 
//////                                                 (long long)cd_id.object_id_, 
//////                                                 (long long)cd_id.sequential_id_, addr);
//////#endif
////    }
//#ifdef _WORK 
 
    // This generates a unique file name that will be used for preservation. 
    // Per CD must be different, so we can refer to CDID and 
    // also HEX address of the pointer we are preserving. 
    // -> This might not be a good thing when we recover actually the stack content can be different... 
    // is it? or is it not?  let's assume it does...
    static char* GetUniqueCDFileName(CDID &cd_id, void *addr, const char* base_=0) 
    {
      char* filename = new char(100);
      const char* base_file_path = base_;
      if(base_file_path == 0) base_file_path = GetCDFilesPath();
      printf("%s\n", base_); 
      printf("%s\n", base_file_path);
      getchar(); 
      sprintf(filename, "%s%lld.%lld.%lld.%lld.%lld.cd", base_file_path, 
                                              (long long)cd_id.domain_id_, 
                                              (long long)cd_id.node_id_.task_, 
                                              (long long)cd_id.level_, 
                                              (long long)cd_id.object_id_, 
                                              (long long)cd_id.sequential_id_);
      printf("%s\n", filename);
      getchar(); 
      return filename;
    }
//#endif

/*
    static void GetUniqueCDFileName(CDID &cd_id, void *addr, char *filename)
    { // This generates a unique file name that will be used for preservation. Per CD must be different, so we can refer to CDID and also 
      //hex address of the pointer we are preserving. -> This might not be a good thing when we recover actually the stack content can be different... is it? or is it not?  let's assume it does...

//      const char *base_file_path = GetCDFilesPath(cd_id);	
      const char* base_file_path = GetCDFilesPath();
//#ifdef GONG
			// GONG
      sprintf(filename, "%s%lld.%lld.%lld.%lld.cd", 
							base_file_path, (long long)cd_id.domain_id_,  
							(long long)cd_id.level_, (long long)cd_id.object_id_, (long long)cd_id.sequential_id_);
//#else
//      sprintf(filename, "%s%lld.%lld.%lld.%lld.%p.cd", 
//							base_file_path, (long long)cd_id.domain_id_,  
//							(long long)cd_id.level_, (long long)cd_id.object_id_, (long long)cd_id.sequential_id_, addr);
//#endif
    }
//#ifdef GONG		
		//GONG
    static void GetUniqueCDFileName(CDID &cd_id, void *addr, char *filename, std::string base_)
    { // This generates a unique file name that will be used for preservation. Per CD must be different, so we can refer to CDID and also 
      //hex address of the pointer we are preserving. -> This might not be a good thing when we recover actually the stack content can be different... is it? or is it not?  let's assume it does...

//      const char *base_file_path = GetCDFilesPath(cd_id);	
      const char* base_file_path = base_.c_str();
      sprintf(filename, "%s%lld.%lld.%lld.%lld.cd", 
							base_file_path, (long long)cd_id.domain_id_,  
							(long long)cd_id.level_, (long long)cd_id.object_id_, (long long)cd_id.sequential_id_);

    }
//#endif
*/

};

class Path {
  public:
  	std::string _path_SSD;
  	std::string _path_HDD;
	public:
  	Path() {
  		_path_SSD = "./SSD";
  		_path_HDD = "./HDD";
  	}
  	Path(std::string ssd, std::string hdd) {
  		_path_SSD = ssd;
  		_path_HDD = hdd;
  	}
  	void SetSSDPath(std::string path_SSD) { _path_SSD = path_SSD; }
  	void SetHDDPath(std::string path_HDD) { _path_HDD = path_HDD; }
  	std::string GetSSDPath(void)          { return _path_SSD;     }
  	std::string GetHDDPath(void)          { return _path_HDD;     }
  	Path& operator=(const Path& that) {
  		_path_SSD = that._path_SSD;
  		_path_HDD = that._path_HDD;
  		return *this;
  	}
};


#endif
