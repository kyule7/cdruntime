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
#include <map>
#include <initializer_list>
#include <iostream>
#define CD_VECTOR_PRINT(...)



namespace cd {


static std::map<std::string, uint64_t> str2id;

template <typename T>
class CDVector : public std::vector<T>, public PackerSerializable {
//  int orig_size;
  public:
  void *Serialize(uint64_t &len_in_bytes) { return NULL; }
  void Deserialize(void *object) {}
  uint64_t PreserveObject(packer::DataStore *packer) {
//    CD_VECTOR_PRINT("CDVector Preserve elemsize:%zu, %zu %zu\n", sizeof(T), this->size(), this->capacity()); 
//    char *ptr = reinterpret_cast<char *>(this->data());
//    packer.Add(ptr, packer::CDEntry(id_, this->size() * sizeof(T), 0, ptr));
    
    return 0; 
  
  }
  uint64_t PreserveObject(packer::CDPacker &packer, const std::string &entry_name) {
    CD_VECTOR_PRINT("CDVector Preserve elemsize:%zu, %zu %zu\n", sizeof(T), this->size(), this->capacity()); 
    if(myTaskID == 0) {
    printf("CDVector [%s] Preserve elemsize:%zu, %zu %zu\n", entry_name.c_str(), sizeof(T), this->size(), this->capacity()); 
    }
    id_ = GetCDEntryID(entry_name);
    //CheckID(entry_name); id_ = str2id[entry_name];
//    printf("id:%lx\n", id_);
    char *ptr = reinterpret_cast<char *>(this->data());
    uint64_t prv_size = this->size() * sizeof(T);
    packer::CDEntry entry(id_, prv_size, 0, ptr);
    packer.Add(ptr, entry);//packer::CDEntry(id_, this->size() * sizeof(T), 0, ptr));
    return prv_size; 
  
  }
  uint64_t Deserialize(packer::CDPacker &packer, const std::string &entry_name) {
    CD_VECTOR_PRINT("CDVector Restore elemsize:%zu, %zu %zu\n", sizeof(T), this->size(), this->capacity()); 
    id_ = GetCDEntryID(entry_name);
    //id_ = cd_hash(entry_name);
    //GetCDEntryID(entry_name.c_str());
    //CheckID(entry_name); id_ = str2id[entry_name];
    char *ptr = reinterpret_cast<char *>(this->data());
    packer::CDEntry *pentry = reinterpret_cast<packer::CDEntry *>(packer.Restore(id_, ptr, this->size() * sizeof(T)));
    if(pentry->size() == 0) {
      this->clear();
    } else {
      this->resize(pentry->size() / sizeof(T));
    }
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
//  virtual int Preserv(packer::CDPacker &packer) { CD_VECTOR_PRINT("CDVector Preserv \n"); return 0; }
//  virtual int Restore(packer::CDPacker &packer) { CD_VECTOR_PRINT("CDVector Restore \n"); return 0; }
  CDVector<T>(void) { Init(); }
  CDVector<T>(int len) : std::vector<T>(len) { Init(); }
  CDVector<T>(const std::initializer_list<T> &il) : std::vector<T>(il) { Init(); }

  void Init(void) {
    static bool init_str2id = false;
    if(init_str2id == false) {
      str2id["X"        ]   = 0xffef00;
      str2id["Y"        ]   = 0xffef01;
      str2id["Z"        ]   = 0xffef02;
      str2id["XD"       ]   = 0xffef03;
      str2id["YD"       ]   = 0xffef04;
      str2id["ZD"       ]   = 0xffef05;
      str2id["XDD"      ]   = 0xffef06;
      str2id["YDD"      ]   = 0xffef07;
      str2id["ZDD"      ]   = 0xffef08;
      str2id["FX"       ]   = 0xffef09;
      str2id["FY"       ]   = 0xffef10;
      str2id["FZ"       ]   = 0xffef11;
      str2id["NODALMASS"]   = 0xffef12;
      str2id["SYMMX"    ]   = 0xffef13;
      str2id["SYMMY"    ]   = 0xffef14;
      str2id["SYMMZ"    ]   = 0xffef15;
      str2id["NODELIST" ]   = 0xffef16;
      str2id["LXIM"     ]   = 0xffef17;
      str2id["LXIP"     ]   = 0xffef18;
      str2id["LETAM"    ]   = 0xffef19;
      str2id["LETAP"    ]   = 0xffef20;
      str2id["LZETAM"   ]   = 0xffef21;
      str2id["LZETAP"   ]   = 0xffef22;
      str2id["ELEMBC"   ]   = 0xffef23;
      str2id["DXX"      ]   = 0xffef24;
      str2id["DYY"      ]   = 0xffef25;
      str2id["DZZ"      ]   = 0xffef26;
      str2id["DELV_XI"  ]   = 0xffef27;
      str2id["DELV_ETA" ]   = 0xffef28;
      str2id["DELV_ZETA"]   = 0xffef29;
      str2id["DELX_XI"  ]   = 0xffef30;
      str2id["DELX_ETA" ]   = 0xffef31;
      str2id["DELX_ZETA"]   = 0xffef32;
      str2id["E"        ]   = 0xffef33;
      str2id["P"        ]   = 0xffef34;
      str2id["Q"        ]   = 0xffef35;
      str2id["QL"       ]   = 0xffef36;
      str2id["QQ"       ]   = 0xffef37;
      str2id["V"        ]   = 0xffef38;
      str2id["VOLO"     ]   = 0xffef39;
      str2id["VNEW"     ]   = 0xffef40;
      str2id["DELV"     ]   = 0xffef41;
      str2id["VDOV"     ]   = 0xffef42;
      str2id["AREALG"   ]   = 0xffef43;
      str2id["SS"       ]   = 0xffef44;
      str2id["ELEMMASS" ]   = 0xffef45;
      str2id["REGELEMSIZE"] = 0xffef46;
      str2id["REGNUMLIST" ] = 0xffef47;
      str2id["REGELEMLIST"] = 0xffef48;
      str2id["COMMBUFSEND"] = 0xffef49;
      str2id["COMMBUFRECV"] = 0xffef50;
      init_str2id = true;
    }
  }
  void CheckID(const std::string &str) {
    auto it = str2id.find(str);
    static int cnt = 0;
    if(it == str2id.end()) {
      str2id[str] = 0xffef50 + cnt++;
    }
    if(myTaskID == 0) {
    
      printf("%s id:%s, %lu\n", __func__, str.c_str(), str2id[str]);
    }
  }

};


}

#endif
