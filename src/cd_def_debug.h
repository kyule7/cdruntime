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

// DEBUG related
#define ERROR_MESSAGE(...) \
  { fprintf(stderr, __VA_ARGS__); assert(0); }

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

#define DEBUG_OFF_MAILBOX 0
#define DEBUG_OFF_ERRORINJ 0
#define DEBUG_OFF_PACKER 1

#define CD_DEBUG_TRACE_INFO(stream, ...) \
  cd_debug_trace(stream, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#if CD_DEBUG_DEST == CD_DEBUG_SILENT  // No printouts 

#define CD_DEBUG(...) 
#define CD_DEBUG_COND(...)
#define CD_DEBUG_FLUSH
#define LOG_DEBUG(...) 
#define LIBC_DEBUG(...)
 
#elif CD_DEBUG_DEST == CD_DEBUG_TO_FILE  // Print to fileout

extern FILE *cdout;
extern FILE *cdoutApp;

#define CD_DEBUG(...) \
  CD_DEBUG_TRACE_INFO(cdout, __VA_ARGS__)

#define CD_DEBUG_COND(DEBUG_OFF, ...) \
if(DEBUG_OFF == 0) { CD_DEBUG_TRACE_INFO(cdout, __VA_ARGS__); }

#define CD_DEBUG_FLUSH \
  fflush(cdout)

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
if(DEBUG_OFF == 0) { CD_DEBUG_TRACE_INFO(cdout, __VA_ARGS__); }

#define CD_DEBUG_FLUSH 

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

#define CD_DEBUG_FLUSH

#else  // -------------------------------------

#define CD_DEBUG(...) \
  CD_DEBUG_TRACE_INFO(stderr, __VA_ARGS__)

#define CD_DEBUG_FLUSH
#endif







#endif // file ends
