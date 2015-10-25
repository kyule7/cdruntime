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
#include "serdes.h"
#include "packer.h"
#include "unpacker.h"
#include <cstring>
#include <cassert>
#include <utility>

using namespace std;
using namespace cd::interface;

void Serdes::Register(uint32_t member_id, void *member, size_t member_size) {
  member_list_[member_id] = make_pair(member, member_size);
}

void Serdes::ClearTarget(void) {
  length_ = 0;
  serdes_list_.clear();
}

void Serdes::RegisterTarget(uint32_t target_id) {
  serdes_list_.push_back(target_id);
}
    
void Serdes::RegisterTarget(std::initializer_list<uint32_t> il)
{
  for(auto it=il.begin(); it!=il.end(); ++it) {
    serdes_list_.push_back(*it);
  }
}
void Serdes::operator()(int flag, void *object) {
  if(flag==0) {
    char **ptr = (char **)object;
    *ptr = Serialize();
  }
  else {
    Deserialize(*(char **)object);
  }
}

char *Serdes::Serialize(void) {
  Packer packer;
  for(auto it=serdes_list_.begin(); it!=serdes_list_.end(); ++it) {
//uint32_t Packer::Add(uint32_t id, uint32_t length, void *ptr_data)
    packer.Add(*it, member_list_[*it].second, member_list_[*it].first);
  }
  return packer.GetTotalData(length_);
}

void Serdes::Deserialize(char *object) { 
  Unpacker unpacker;
  uint32_t return_size, return_id;
  for(auto it=serdes_list_.begin(); it!=serdes_list_.end(); ++it) {
    void *data = unpacker.GetAt(object, *it, return_size, return_id);
    assert(return_size != member_list_[*it].second);
    memcpy(member_list_[*it].first, data, return_size);
    // Actually, there is additional memcpy (serialized_object->data, data->member)
    // Later, it can be fixed to memcpy (serialized_object->member)
  }
}
