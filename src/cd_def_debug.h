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

#ifndef _CD_DEF_DEBUG_H
#define _CD_DEF_DEBUG_H

#include "cd_features.h"
#include "cd_global.h"
#include <cstdarg>
#include <cstdio>
#include <sstream>
#include <fstream>

namespace cd {

// DEBUG related
#define ERROR_MESSAGE(...) \
  { fprintf(stderr, __VA_ARGS__); fflush(stderr); assert(0); }

#define CD_ASSERT(...) assert(__VA_ARGS__);

#define CD_ASSERT_STR(COND, ...) \
  { if((COND) == false) { printf(__VA_ARGS__); } assert(COND); }

/* Eric:  Should be using vsnprintf to a buffer with explicit flush, 
 *        Add a comment to this line rather than a pair of fprintf's 
 *        to format the strings.
 *
 * The major disadvantage of two printf's is that they can be mixed
 * up when multiple threads/ranks are writing to the same output, confusing
 * the output.
 */
static inline 
int cd_debug_trace(FILE *stream, const char *source_file,
                                 const char *function, int line_num,
                                 const char *fmt, ...)
{
    int bytes;
    va_list argp;
//    bytes = fprintf(stream, "%s:%d: %s: ", source_file, line_num, function);
    bytes = fprintf(stream, "[%s] ", function);
    va_start(argp, fmt);
    bytes += vfprintf(stream, fmt, argp);
    va_end(argp);
    fflush(stream);
    return bytes;
}

#define CD_DEBUG_TRACE_INFO(stream, ...) \
  cd_debug_trace(stream, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__);

extern FILE *cdout;
extern FILE *cdoutApp;

// Set up debug printouts
#if CD_DEBUG_DEST == CD_DEBUG_SILENT  // No printouts 

#define CD_DEBUG(...) 
#define CD_DEBUG_COND(...)
#define LOG_DEBUG(...) 
#define LIBC_DEBUG(...)
 
#elif CD_DEBUG_DEST == CD_DEBUG_TO_FILE  // Print to fileout

extern FILE *cdout;
extern FILE *cdoutApp;

#define CD_DEBUG(...) \
  CD_DEBUG_TRACE_INFO(cdout, __VA_ARGS__)

#define CD_DEBUG_COND(DEBUG_OFF, ...) \
if(DEBUG_OFF == 0) { CD_DEBUG_TRACE_INFO(cdout, __VA_ARGS__); }

#define LOG_DEBUG(...) /*\
  { if(cd::app_side) {\
      cd::app_side=false;\
      CD_DEBUG_TRACE_INFO(cdout, __VA_ARGS__);\
      cd::app_side = true;}\
    else CD_DEBUG_TRACE_INFO(cdout, __VA_ARGS__);\
  }*/

#define LIBC_DEBUG(...) /*\
    { if(cd::app_side) {\
        cd::app_side=false;\
        CD_DEBUG_TRACE_INFO(stdout, __VA_ARGS__);\
        cd::app_side = true;}\
      else CD_DEBUG_TRACE_INFO(stdout, __VA_ARGS__);\
    }*/



#elif CD_DEBUG_DEST == CD_DEBUG_STDOUT  // print to stdout 

#define CD_DEBUG(...) \
  CD_DEBUG_TRACE_INFO(stdout, __VA_ARGS__)

#define CD_DEBUG_COND(DEBUG_OFF, ...) \
if(DEBUG_OFF == 0) { CD_DEBUG_TRACE_INFO(stdout, __VA_ARGS__); }


#define LOG_DEBUG(...) /*\
  { if(cd::app_side) {\
      cd::app_side=false;\
      CD_DEBUG_TRACE_INFO(stdout, __VA_ARGS__);\
      cd::app_side = true;}\
    else CD_DEBUG_TRACE_INFO(stdout, __VA_ARGS__);\
  }*/

#define LIBC_DEBUG(...)/* \
    { if(cd::app_side) {\
        cd::app_side=false;\
        CD_DEBUG_TRACE_INFO(stdout, __VA_ARGS__);\
        cd::app_side = true;}\
      else CD_DEBUG_TRACE_INFO(stdout, __VA_ARGS__);\
    }*/


#elif CD_DEBUG_DEST == CD_DEBUG_STDERR  // print to stderr

#define CD_DEBUG(...) \
  CD_DEBUG_TRACE_INFO(stderr, __VA_ARGS__)

#define CD_DEBUG_COND(DEBUG_OFF, ...) \
if(DEBUG_OFF == 0) { CD_DEBUG_TRACE_INFO(cdout, __VA_ARGS__); }


#else  // -------------------------------------

#define CD_DEBUG(...) \
  CD_DEBUG_TRACE_INFO(stderr, __VA_ARGS__)

#endif



#define CD_PRINT(...) \
  printf(__VA_ARGS__)
//  CD_DEBUG_TRACE_INFO(cdoutApp, __VA_ARGS__)


///**@class cd::DebugBuf
// * @brief Utility class for debugging.
// *
// *  Like other parallel programming models, it is hard to debug CD runtime. 
// *  So, debugging information are printed out to file using this class.
// *  The global object of this class, dbg, is defined.
// * 
// *  The usage of this class is like below.
// *  \n
// *  \n
// *  DebugBuf dbgApp;
// *  dbgApp.open("./file_path_to_write"); 
// *  dbgApp << "Debug Information" << endl;
// *  dbg.flush(); 
// *  dbg.close();
// */ 
//  class DebugBuf: public std::ostringstream {
//    std::ofstream ofs_;
//  public:
//    ~DebugBuf() {}
//    DebugBuf(void) {}
//    DebugBuf(const char *filename) 
//      : ofs_(filename) {}
///**
// * @brief Open a file to write debug information.
// *
// */
//    void open(const char *filename)
//    {
//      bool temp = app_side;
//      app_side = false;
//      ofs_.open(filename);
//      app_side = temp;
//    }
//
///**
// * @brief Close the file where debug information is written.
// *
// */    
//    void close(void)
//    {
//      bool temp = app_side;
//      app_side = false;
//      ofs_.close();
//      app_side = temp;
//    }
//
// 
///**
// * @brief Flush the buffer that contains debugging information to the file.
// *        After flushing it, it clears the buffer for the next use.
// *
// */
//    void flush(void) 
//    {
//      bool temp = app_side;
//      app_side = false;
//      ofs_ << str();
//      ofs_.flush();
//      str("");
//      clear();
//      app_side = temp;
//    }
//     
//
//    template <typename T>
//    std::ostream &operator<<(T val) 
//    {
//      bool temp = app_side;
//      app_side = false;
//      std::ostream &os = std::ostringstream::operator<<(val);
//      app_side = temp;
//      return os;
//    }
// 
//  };

//#if CD_DEBUG_ENABLED
//  extern std::ostringstream dbg;
//  extern DebugBuf cddbg;
#define dbgBreak nullFunc
//#else
//#define cddbg std::cout
//#define dbgBreak nullFunc
//#endif

  } // namespace cd ends
#endif // file ends
