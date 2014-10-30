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

#ifndef _CD_REQ_H 
#define _CD_REQ_H

namespace cd {

class  : public std::vector<CDHandle*> {
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


}
