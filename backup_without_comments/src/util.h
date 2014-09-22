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
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>
#include "cd_id.h"
// system specific or machine specific utils resides here.
using namespace cd;

namespace cd {

class StringBuffer : public std::string {
  std::ostringstream _stream;
public:
  StringBuffer() {}
  StringBuffer(const std::string& initTxt) {
  	_stream << initTxt;
    assign(_stream.str());
  }
  
  template <typename T>
  StringBuffer& operator<<(T const& t) {
    _stream << t;
    assign(_stream.str());
    return *this;
  }

  std::string GetString() const { 
    return _stream.str(); 
  }
 
};


class Util {
public:

static const char* GetBaseFilePath()
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
static std::string GetUniqueCDFileName(const CDID& cd_id, const char* basepath, const char* data_name) 
{
//  std::string base(GetBaseFilePath());
  std::string base(basepath);
  StringBuffer filename(base);
//  StringBuffer filename(std::string(base));
//

//  std::cout << "base file name: "<< filename.GetString() << std::endl << std::endl;
  filename << cd_id.level_ << '.' << cd_id.rank_in_level_ << '.' << cd_id.object_id_ << '.' << cd_id.sequential_id_ << '.' << cd_id.node_id_.task_in_color_ << '.' << data_name << ".cd";
//  std::cout << "file name for this cd: "<< filename.GetString() << std::endl << std::endl; getchar(); 
  return filename.GetString();
//  return "./";
}

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

};

//class Path {
//  public:
//  	std::string _path_SSD;
//  	std::string _path_HDD;
//	public:
//  	Path() {
//  		_path_SSD = "./SSD";
//  		_path_HDD = "./HDD";
//  	}
//  	Path(std::string ssd, std::string hdd) {
//  		_path_SSD = ssd;
//  		_path_HDD = hdd;
//  	}
//  	void SetSSDPath(std::string path_SSD) { _path_SSD = path_SSD; }
//  	void SetHDDPath(std::string path_HDD) { _path_HDD = path_HDD; }
//  	std::string GetSSDPath(void)          { return _path_SSD;     }
//  	std::string GetHDDPath(void)          { return _path_HDD;     }
//  	Path& operator=(const Path& that) {
//  		_path_SSD = that._path_SSD;
//  		_path_HDD = that._path_HDD;
//  		return *this;
//  	}
//};

} // namespace cd ends

#endif
