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
#ifndef _CD_CONTAINERS_H
#define _CD_CONTAINERS_H

#include "serializable.h"
#include "cd_packer.hpp"
#include <vector>
#include <initializer_list>
#include <iostream>

namespace cd {

template <typename T>
class CDVector : public std::vector<T>, public PackerSerializable {
//  int orig_size;
  public:
  void *Serialize(uint64_t &len_in_bytes) {}
  void Deserialize(void *object) {}
  uint64_t PreserveObject(packer::DataStore *packer) {
//    printf("CDVector Preserve elemsize:%zu, %zu %zu\n", sizeof(T), this->size(), this->capacity()); 
//    char *ptr = reinterpret_cast<char *>(this->data());
//    packer.Add(ptr, packer::CDEntry(id_, this->size() * sizeof(T), 0, ptr));
    
    return 0; 
  
  }
  uint64_t PreserveObject(packer::CDPacker &packer, const std::string &entry_name) {
    printf("CDVector Preserve elemsize:%zu, %zu %zu\n", sizeof(T), this->size(), this->capacity()); 
    id_ = GetCDEntryID(entry_name.c_str());
    char *ptr = reinterpret_cast<char *>(this->data());
    packer.Add(ptr, packer::CDEntry(id_, this->size() * sizeof(T), 0, ptr));
    return 0; 
  
  }
  uint64_t Deserialize(packer::CDPacker &packer, const std::string &entry_name) {
    printf("CDVector Restore elemsize:%zu, %zu %zu\n", sizeof(T), this->size(), this->capacity()); 
    id_ = GetCDEntryID(entry_name.c_str());
    char *ptr = reinterpret_cast<char *>(this->data());
    packer::CDEntry *pentry = reinterpret_cast<packer::CDEntry *>(packer.Restore(id_, ptr, this->size() * sizeof(T)));
    this->resize(pentry->size() / sizeof(T));
    return 0; 
  
  }
  void Print(const char str[]="") {
    std::cout << str << " ptr: "<< this->data() 
                << ", vec size:" << this->size() 
                << ", vec cap: " << this->capacity() 
                << ", vec obj size: " << sizeof(*this) << std::endl;
    int size = this->size();
    int stride = 8;
    if(size > stride) {
      int numrows = size/stride;
      for(int i=0; i<numrows; i++) {
        for(int j=0; j<stride; j++) {
//          std::cout << "index: "<<  j + i*stride <<", "<< size/8 << std::endl;
          std::cout << this->at(j+i*stride) << '\t';
        }
        std::cout << std::endl;
      }
    } else {
      for(int i=0; i<size; i++) {
        std::cout << this->at(i) << '\t';
      }
    }

    std::cout << std::endl;
  }
//  virtual int Preserv(packer::CDPacker &packer) { printf("CDVector Preserv \n"); return 0; }
//  virtual int Restore(packer::CDPacker &packer) { printf("CDVector Restore \n"); return 0; }
  CDVector<T>(void) {}
  CDVector<T>(int len) : std::vector<T>(len) {}
  CDVector<T>(const std::initializer_list<T> &il) : std::vector<T>(il) {}
};


}

#endif
