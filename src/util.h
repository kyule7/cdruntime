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

/**
 * @file util.h
 * @author Jinsuk Chung, Kyushick Lee
 * @date March 2015
 *
 * \brief Utilities for CD runtime
 *
 */
#include "cd_global.h"
#include "cd_handle.h"
#include "cd_id.h"

#include <sys/types.h>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>

// system specific or machine specific utils resides here.
using namespace cd;

namespace cd {
/** \addtogroup utilities Utilities for CD runtime
 *
 *@{
 *
 * @brief Read time at a point of time
 *
 */ 
static __inline__ 
uint64_t rdtsc(void)
{
  unsigned high, low;
  __asm__ __volatile__ ("rdtsc" : "=a"(low), "=d"(high));
  return ((uint64_t)low) | (((uint64_t)high) << 32);
}

class Util {
public:

/** 
 * @brief Generate unique name to write to filesystem.
 *
 * FIXME This should return appropriate path depending on configuration 
 * and perhaps each node may have different path to store CD related files
 */
static const char* GetBaseFilePath(void)
{
  return "./"; 
}

/** 
 * @brief Generate unique name to write to filesystem.
 *
 * This generates a unique file name that will be used for preservation. 
 * Per CD must be different, so we can refer to CDID and 
 * also HEX address of the pointer we are preserving. 
 * -> This might not be a good thing when we recover actually the stack content can be different... 
 * is it? or is it not?  let's assume it does...
 */ 
static std::string GetUniqueCDFileName(const CDID &cd_id, const char *basepath, const char *data_name) 
{
  std::string base(basepath);
  
  std::ostringstream filename(base);

//  std::cout << "base file name: "<< base << std::endl; //filename.str() << std::endl << std::endl;
//  filename << cd_id.level() << '.' << cd_id.rank_in_level() << '.' << cd_id.object_id() << '.' << cd_id.sequential_id() << '.' << cd_id.task_in_color() << '.' << data_name << ".cd";
  base += std::to_string(cd_id.level())         + std::string(".") 
        + std::to_string(cd_id.rank_in_level()) + std::string(".") 
        + std::to_string(cd_id.object_id())     + std::string(".") 
        + std::to_string(cd_id.sequential_id()) + std::string(".") 
        + std::to_string(cd_id.task_in_color()) + std::string(".") 
        + std::string(data_name)                + std::string(".cd");
//  std::cout << "file name for this cd: "<< base << std::endl; // filename.str() << std::endl << std::endl; //dbgBreak(); 
//  return filename.str();
  return base;
}

/** 
 * @brief PFS-related
 *
 */ 
static std::string GetUniqueCDFileName(const CDID& cd_id, const char* basepath, const char* data_name, const PrvMediumT preservationMedium )
{  
  std::string base(basepath);
  std::ostringstream filename(base);
  
  if( (preservationMedium == kHDD) || (preservationMedium == kSSD) ) {
//    filename << cd_id.level() << '.' << cd_id.rank_in_level() << '.' << cd_id.object_id() << '.' << cd_id.sequential_id() << '.' << cd_id.task_in_color() << '.' << data_name << ".cd";
    base += std::to_string(cd_id.level())         + std::string(".") 
          + std::to_string(cd_id.rank_in_level()) + std::string(".") 
          + std::to_string(cd_id.object_id())     + std::string(".") 
          + std::to_string(cd_id.sequential_id()) + std::string(".") 
          + std::to_string(cd_id.task_in_color()) + std::string(".") 
          + std::string(data_name)                + std::string(".cd");
  }
  else if( preservationMedium == kPFS ) { 
//    filename << cd_id.level() << '.' << cd_id.rank_in_level() << '.' << cd_id.sequential_id();
    base += std::to_string(cd_id.level()) + std::string(".") 
          + std::to_string(cd_id.rank_in_level()) + std::string(".") 
          + std::to_string(cd_id.sequential_id());
  }
  else {
    //This case is ERROR.
    ERROR_MESSAGE("We should not get here! there is something wrong.\n");
  }
  std::cout << "GenPath : " << filename.str() << ", base : " << base << endl;
  //return filename.str();
  return base;
}

/** 
 * @brief Get process ID of current task.
 *
 */
static pid_t GetCurrentProcessID()
{
  //FIXME maybe it is the same task id 
  return getpid();	
}

/** 
 * @brief Get Domain ID of current task in the CD.
 *
 */
static uint64_t GetCurrentDomainID()
{
  //STUB
  return 0;
}


/** 
 * @brief Generate object ID for CD object
 *
 *
 * The policy for Generating CDID could be different . 
 * But this should be unique
 * FIXME for multithreaded version this is not safe 
 * Assume we call this function one time, so we will have atomically increasing counter and this will be local to a process. 
 * Within a process it will just use one counter. Check whether this is enough or not.
 * static uint64_t object_id=0;
 *
 */ 
static uint64_t GenCDObjID() 
{
  return gen_object_id++;
}

static std::map<uint32_t, uint32_t> object_id;

static uint64_t GenCDObjID(uint32_t level) 
{
  return object_id[level]++;
}

};

/** @} */ // End group utilities

} // namespace cd ends

#endif
