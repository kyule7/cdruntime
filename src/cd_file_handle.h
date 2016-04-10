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


#ifndef _CD_LOG_HANDLE_H
#define _CD_LOG_HANDLE_H

/**
 * @file cd_log_handle.h
 * @author Kyushick Lee, Song Zhang, Seong-Lyong Gong, Ali Fakhrzadehgan, Jinsuk Chung, Mattan Erez
 * @date March 2015
 *
 * @brief Containment Domains API v0.2 (C++)
 */

#include "cd_global.h"
#include "cd_def_internal.h"
#include "cd_def_preserve.h"
namespace cd {
  namespace internal {

class CDFileHandle;

//class FilePath {
//  friend class CD;
//  friend class CDFileHandle;
//  private:
//    std::string filename_;
//    std::string basepath_;
//  public:
//    static std::string global_prv_path_;
//  private:
//    FilePath(void);
//    FilePath(const std::string &basepath, 
//             const std::string &filename); 
//    std::string GetFilePath(void);
//    void SetFilePath(const std::string &basepath, 
//                     const std::string &filename);
//  
//    // assignment
//    FilePath &operator=(const FilePath &that) {
//      basepath_ = that.basepath_;
//      filename_ = that.filename_;
//      return *this;
//    }
//  
//    // assignment
//    FilePath &operator=(const char *basepath_) {
//      basepath_ = basepath_;
//      return *this;
//    }
//};



class CDFileHandle {
  friend cd::CDHandle *cd::CD_Init(int numTask, int myTask, PrvMediumT prv_medium);
  friend class CD;
  friend class HeadCD;
  friend class CDEntry;
    int file_desc_;
    FILE *fp_;
    long fpos_generator_;
    bool opened_;
    char unique_filename_[MAX_FILE_PATH];
    std::string fullpath_;
    static std::string global_prv_path_;
    static std::string local_prv_path_;
  public:
    CDFileHandle(void);
    CDFileHandle(const PrvMediumT& prv_medium, 
                 const std::string &filename);
    ~CDFileHandle(void);
    void Initialize(const PrvMediumT& prv_medium, 
                    const std::string &filename);
    void Close(void);
    void InitOpenFile(void) { opened_ = false; }
    bool IsOpen(void) { return opened_; }
    long UpdateFilePos(long offset) 
    { 
      // For multi-threaded env, this needs to be atomic
      return fpos_generator_ += offset; 
    }
    void OpenFilePath(void);     
    char *GetFilePath(void);
    void SetFilePath(const PrvMediumT& prv_medium, 
                     const std::string &filename);
};



  } // namespace internal ends
} // namespace cd ends
#endif
