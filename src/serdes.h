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
#ifndef _SERDES_H 
#define _SERDES_H

#include "cd_features.h"
//#include "cd_handle.h"

#include <list>
#include <map>
#include <initializer_list>
/*
// User needs to define Serdes methods in his/her class
// User defines this in his/her class.
class UsrObj {
  // Members
  map<int, void *> member_list_;
  Serdes serdes_;

  UsrObj() {
    // ...
    // User need to register each member to member_list_ table
    serdes_.Regsiter(member0_ID,&member0,member0.size());
    serdes_.Regsiter(member1_ID,&member1,member1.size());
    serdes_.Regsiter(member2_ID,&member2,member2.size());
    serdes_.Regsiter(member3_ID,&member3,member3.size());
    // ...
  }
}
 *
 */

namespace cd {

  class CDHandle;

  namespace interface {


class Serdes {
  friend cd::CDHandle; // Only CDHandle can invoke operator()
    std::list<uint32_t> serdes_list_;
    std::map<uint32_t, std::pair<void *, size_t>> member_list_;
    uint32_t length_;
  public:
    void Register(uint32_t member_id, void *member, size_t member_size);
    void ClearTarget(void);
    void RegisterTarget(uint32_t target_id);
    void RegisterTarget(std::initializer_list<uint32_t> il);
  private:
    // This will be invoked later by CD runtime
    void operator()(int flag, void *object);
    char *Serialize(void);
    void Deserialize(char *object);
};

  }
}


#endif
