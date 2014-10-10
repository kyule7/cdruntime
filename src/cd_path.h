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

#ifndef _CD_PATH_H 
#define _CD_PATH_H

#include <vector>
#include "cd_handle.h"
#include "cd_entry.h"
namespace cd {

//class CDHandle;

// TODO
// Using DCL(Double-Checking Locking), try to reduce some synch overhead for GetCDPath for multiple threads.
// DCL will check if uniquePath_ is created or not, it performs synch only when it is not created before.
// Using volatile keyword, we can let uniquePath_ be initialized correctly for multi-threading.

// Kyushick : CDPath object should be unique for one process, so I adopted Singleton pattern for it.
// But the difference is that I modified it for something like Memento pattern for it also, 
// because it also performs like CareTaker object in that pattern.
// this case CareTaker object manages the previous CDHandle*. 
// CDHandle is mapped to Originator which is an interface
// and Memento object is mapped to CD object
// Actually our design is not exactly the same as Memento pattern,
// in that user nodifies when to restore explicitly in program as well in Memento pattern,
// but in our case, we restore program state inside our runtime system which is implicit to user (user does not know about when error happens)
// I put this CDPath as a Class rather than just static global variable, so it can be more object-oriented
// because related methods such as GetCurrentCD, GetRootCD, etc, is encapsulated in this CDPath class.
// It will be more convenient to use it with "using namespace cd::CDPath", and the usage for those methods will be the same as before.
class CDPath : public std::vector<CDHandle*> {
private:
  static CDPath* uniquePath_;
private:
  CDPath(void) {}

public:
  static CDPath* GetCDPath(void) 
  {
    if(uniquePath_ == NULL) 
      uniquePath_ = new CDPath();
    return uniquePath_;
  }

  static CDHandle* GetCurrentCD(void) 
  { 
    if(uniquePath_ != NULL ) {
      if( !uniquePath_->empty() ) {
        return uniquePath_->back(); 
      }
    }
    return NULL;
  }
  
  static CDHandle* GetRootCD(void)    
  { 
    if(uniquePath_ != NULL) { 
      if( !uniquePath_->empty() ) {
        return uniquePath_->front(); 
      }
    }
    return NULL;
  }
  
  static CDHandle* GetParentCD(void)
  { 
    if(uniquePath_ != NULL) {
      if( uniquePath_->size() > 1 ) {
        return uniquePath_->at(uniquePath_->size()-2); 
      }
    }
    return NULL;
  }

  static CDHandle* GetParentCD(int current_level)
  { 
    if(uniquePath_ != NULL ) {
      if( uniquePath_->size() > 1 ) {
        return uniquePath_->at(current_level-2); 
      }
    }
    return NULL;
  }

};




/*
class CDPath {
private:
//  volatile static CDPath uniquePath_;
  static std::atomic<CDPath*> uniquePath_;
  static std::mutex mutex_;
  CDPath(void) {}
public:
  static CDPath* GetCDPath(void) {
    CDPath* tmp = uniquePath_.load(std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_acquire);
    if(tmp == nullptr) {
      std::lock_quard<std::mutex> lock(mutex_);
      tmp = uniquePath_.load(std::memory_order_relaxed);
      if(tmp == nullptr) {
        tmp = new CDPath();
        std::atomic_thread_fence(std::memory_order_release);
        uniquePath_.store(tmp, std::memory_order_relaxed);
      }
    }
    return tmp;
  }
};
*/

//CDPath* CDPath::uniquePath_;
} // namespace cd ends


#endif
