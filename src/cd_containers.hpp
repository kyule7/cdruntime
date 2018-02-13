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
#include "cd_global.h"
#include <vector>
#include <map>
#include <initializer_list>
#include <iostream>
#define CD_VECTOR_PRINT(...)
#define DO_COMPARE 1
#define UNDEFINED_NAME "undefined"
#define VECTOR_INIT_NAME "V:"

namespace cd {


static std::map<std::string, uint64_t> str2id;

template <typename T1>
float Compare(T1 *orip, T1 *newp, uint32_t size) {
  float difference = 0.0;
  uint32_t diff_cnt = 0;
  uint32_t i=0;
  if(size != 0) {
    for(; i<size; i++, orip++, newp++) {
      //printf("[%d] i:%u, size:%u, %p == %p\n", myTaskID, i, size, orip, newp);
      if(IsReexec() && *orip != *newp) {
        std::cout << "["<< myTaskID << "] i:" << i << ", size:" << size << ", " << *orip << " == " << *newp << std::endl;
      }
      diff_cnt = (abs(*orip - *newp) > 1e-100)? diff_cnt+1 : diff_cnt;
    }
    difference = (float)(diff_cnt) / size;
  }
  return difference;
}

template <typename T, typename Y>
float Compare(     T &prsv_p,       Y &prsv_sz,
             const T &orig_p, const Y &orig_sz,
             size_t elem_sz, 
             const char *entry_str="", 
             const char *name="") {
  PackerPrologue();
  float difference = 0.0;
  // compare
  if(prsv_p == NULL) {
    prsv_sz = orig_sz;
    prsv_p = (T)malloc(orig_sz * elem_sz);
    printf("First Preservation %s\n", name);
  } else if(prsv_sz < orig_sz) {
    difference = 1.0;
    printf("Similarity %s:1.0 (realloced)\n", name);
    prsv_sz = orig_sz;
    prsv_sz *= 2;
    prsv_p = (T)realloc(prsv_p, prsv_sz * elem_sz);
    memcpy(prsv_p, orig_p, orig_sz);
  } else {
    difference = Compare<T>((T *)prsv_p, (T *)orig_p, orig_sz);
    if(difference > 0.0) {
      if(myTaskID == 0) {
        printf("Similarity %s %s:%f (%lu/%lu)\t",
            entry_str, name, difference, (uint64_t)(orig_sz * elem_sz * difference), orig_sz * elem_sz);
      }
      memcpy(prsv_p, orig_p, orig_sz * elem_sz);
    }
    if(prsv_sz != orig_sz) {
      printf("Preservation size changed %lu->%lu\n", prsv_sz * elem_sz, orig_sz * elem_sz);
      prsv_sz = orig_sz;
    } else if(difference > 0.0) {
      if(myTaskID == 0) {
        printf("\n");
      }
    }
  }
  PackerEpilogue();
  return difference;
}

template <typename T>
class CDVector : public std::vector<T>, public PackerSerializable {
    char *orig_;  
    uint64_t orig_size_;  
    uint64_t prv_size_;  
    float difference_;
    bool  read_;
    std::string name_;
  public:
  const char *GetID(void) { return name_.c_str(); }
  void *Serialize(uint64_t &len_in_bytes) { return NULL; }
  void Deserialize(void *object) {}
  uint64_t PreserveObject(packer::DataStore *packer) {
    PackerPrologue();
//    CD_VECTOR_PRINT("CDVector Preserve elemsize:%zu, %zu %zu\n", sizeof(T), this->size(), this->capacity()); 
//    char *ptr = reinterpret_cast<char *>(this->data());
//    packer.Add(ptr, packer::CDEntry(id_, this->size() * sizeof(T), 0, ptr));
    
    PackerEpilogue();
    return 0; 
  
  }
  //uint64_t PreserveObject(packer::CDPacker &packer, const std::string &name_) {
  uint64_t PreserveObject(packer::CDPacker &packer, CDPrvType prv_type, const char *entry_str) {
    PackerPrologue();
    uint64_t prv_size = this->size() * sizeof(T);
    if(entry_str != NULL && name_ == VECTOR_INIT_NAME) {
      name_ += entry_str;
    }
    CD_VECTOR_PRINT("CDVector Preserve elemsize:%zu, %zu %zu\n", sizeof(T), this->size(), this->capacity()); 
    if(myTaskID == 0) {
      CD_VECTOR_PRINT("CDVector [%s] Preserve elemsize:%zu, size:%zu cap:%zu\n", 
          name_.c_str(), sizeof(T), this->size(), this->capacity()); 
    }
    id_ = GetCDEntryID(name_);
    //CheckID(name_); id_ = str2id[name_];
//    CD_VECTOR_PRINT("id:%lx\n", id_);
    char *ptr = reinterpret_cast<char *>(this->data());
    packer::CDEntry entry(id_, ((prv_type & kRef) == kRef)? packer::Attr::krefer : 0, prv_size, 0, ptr);
    //uint64_t table_offset = packer.Add(ptr, entry);//packer::CDEntry(id_, this->size() * sizeof(T), 0, ptr));
    packer.Add(ptr, entry);
    if(myTaskID == 0) {printf("    ##  CHECK %s ## : ", name_.c_str()); entry.Print(name_.c_str());}
//    if(myTaskID == 0) printf("CDVector %s (%lx) preserved (0x%lx)\n", name_.c_str(), id_, prv_size);
#if DO_COMPARE
    CompareVector();
#endif

#if 0
    if(myTaskID == 0 && (name_ == "X" || name_ == "Y"|| name_ == "Z")) {
      CD_VECTOR_PRINT("Preserve %s at table:%lu, data:%lu, size:%lu\n", name_.c_str(), table_offset, entry.offset(), entry.size());
      char *tmp = new char[entry.size()]();
      uint32_t *check4 = (uint32_t *)tmp;
      CD_VECTOR_PRINT("chk %s: %x %x %x %x %x %x %x %x\n", name_.c_str()
          , *(check4), *(check4+1), *(check4+2), *(check4+3), *(check4+4), *(check4+5), *(check4+6), *(check4+7));

      packer.data_->GetData(tmp, entry.size(), entry.offset());
//      uint32_t *check1 = (uint32_t *)ptr;
      float *check0 = reinterpret_cast<float *>(this->data());
      uint32_t *check1 = reinterpret_cast<uint32_t *>(this->data());
      uint32_t *check2 = (uint32_t *)tmp;
      float *check3 = (float *)tmp;

      

      CD_VECTOR_PRINT("ori %s: %f %f %f %f %f %f %f %f\n", name_.c_str()
          , *(check0), *(check0+1), *(check0+2), *(check0+3), *(check0+4), *(check0+5), *(check0+6), *(check0+7));
      CD_VECTOR_PRINT("ori %s: %x %x %x %x %x %x %x %x\n", name_.c_str()
          , *(check1), *(check1+1), *(check1+2), *(check1+3), *(check1+4), *(check1+5), *(check1+6), *(check1+7));
      CD_VECTOR_PRINT("prv %s: %x %x %x %x %x %x %x %x\n", name_.c_str()
          , *(check2), *(check2+1), *(check2+2), *(check2+3), *(check2+4), *(check2+5), *(check2+6), *(check2+7));
      CD_VECTOR_PRINT("prv %s: %f %f %f %f %f %f %f %f\n", name_.c_str()
          , *(check3), *(check3+1), *(check3+2), *(check3+3), *(check3+4), *(check3+5), *(check3+6), *(check3+7));
      delete [] tmp;
    }
#endif
    PackerEpilogue();
    return prv_size; 
  
  }
  //uint64_t Deserialize(packer::CDPacker &packer, const std::string &name_) {
  uint64_t Deserialize(packer::CDPacker &packer, const char *entry_str=NULL) {
    PackerPrologue();
    uint64_t rst_size = this->size() * sizeof(T);
    CD_VECTOR_PRINT("CDVector Restore elemsize:%zu, %zu %zu\n", sizeof(T), this->size(), this->capacity()); 
    uint64_t id = GetCDEntryID(name_);
    assert(id == id_);
    //id_ = cd_hash(name_);
    //GetCDEntryID(name_.c_str());
    //CheckID(name_); id_ = str2id[name_];

    char *ptr = reinterpret_cast<char *>(this->data());
    //printf("CDVector %s restored (%lu)\n", name_.c_str(), rst_size);
//      if(name_ == "V:SYMMX") {
//        if(myTaskID == 0) {
//          Print(std::cout, "Before Restoration V:SYMMX ");
//        }
//        MPI_Barrier(MPI_COMM_WORLD);
//      }
    packer::CDEntry *pentry = reinterpret_cast<packer::CDEntry *>(packer.Restore(id_, ptr, rst_size));
    if(pentry == NULL) { 
      printf("Failed to restore CDVector %s (%lx) restored (%lu)\n", name_.c_str(), id_, rst_size);
      return -1UL; 
    }
//    assert(pentry);
    uint64_t rst_size_ser = pentry->size();
    if(rst_size_ser == 0) {
      this->clear();
    } else {
      this->resize(rst_size_ser / sizeof(T));
    }
    if(rst_size != rst_size_ser) {
      printf("restored size check: %lu == %lu", rst_size, rst_size_ser); 
      //assert(0);
    }

#if DO_COMPARE
    if(CheckVector(entry_str) > 0.0) {
      if(myTaskID == 0) {
        Print(std::cout, "Restoration Check ");
        printf("Entry Check: "); pentry->Print();
        assert(0);
      }
    }
#endif

    if(myTaskID == 0 && (name_ == "X" || name_ == "Y"|| name_ == "Z")) {
      CD_VECTOR_PRINT("Restore %s at data:%lu, size:%lu\n", name_.c_str(), pentry->offset(), pentry->size());
    }
    PackerEpilogue();
    return rst_size; 
  
  }

  float CheckVector(const char *entry_str=NULL) {
      //printf("[%d] %s %s\n", myTaskID, __func__, entry_str);
      float difference = 0.0;
      if(orig_ != NULL) {
        char *ptr = reinterpret_cast<char *>(this->data());
        difference = Compare<T>((T *)orig_, (T *)ptr, this->size());
        if(difference > 0.0) {
          if(myTaskID==0) printf("[%d] <%s %s> difference=%f\n",myTaskID,  __func__, entry_str, difference);
        }
      }
      return difference;
  }

  // It copies app data to orig_ after comparison.
  // Therefore, if data is updated by app, 
  // orig_ and app data are synch'ed at every CompareVector.
  void CompareVector(const char *entry_str="") {
    PackerPrologue();
    //const char *name = (entry_str == NULL)? name_.c_str() : entry_str;
    const char *name = name_.c_str();
    char *ptr = reinterpret_cast<char *>(this->data());
    uint64_t prv_size = this->size() * sizeof(T);
    if(read_) {
      if(myTaskID == 0)
        printf("Read %s %s\n", entry_str, name);
      read_ = false;
    }
    // compare
    if(orig_ == NULL) {
      orig_size_ = prv_size;
      prv_size_ = prv_size;
      orig_ = (char *)malloc(orig_size_);
      if(myTaskID == 0) printf("First Preservation %s\n", name);
      memcpy(orig_, ptr, prv_size);
    } else if(orig_size_ < prv_size) {
      difference_ = 0.0;
      printf("Similarity %s:1.0 (realloced)\n", name);
      orig_size_ = prv_size;
      orig_size_ *= 2;
      prv_size_ = prv_size;
      orig_ = (char *)realloc(orig_, orig_size_);
      memcpy(orig_, ptr, prv_size);
    } else {
      difference_ = Compare<T>((T *)orig_, (T *)ptr, this->size());
      if(difference_ > 0.0) {
        if(myTaskID == 0) {
          printf("Similarity %s %s:%f (%lu/%lu)\t", entry_str, name, difference_, (uint64_t)(prv_size * difference_),prv_size);
        }
        memcpy(orig_, ptr, prv_size);
      }
      if(prv_size_ != prv_size) {
        printf("Preservation size changed %lu->%lu\n", prv_size_, prv_size);
        prv_size_ = prv_size;
      } else if(difference_ > 0.0) {
        if(myTaskID == 0) {
          printf("\n");
        }
      }
    }
    PackerEpilogue();
  }
  void Print(std::ostream &os, const char str[]="", uint64_t upto=0) {
    PackerPrologue();
    os << str << " " << name_ <<", ptr: "<< this->data() 
                << ", vec size:" << std::hex << this->size() * sizeof(T)
                << ", vec cap: " << std::hex << this->capacity() * sizeof(T)
                << ", vec obj size: " << sizeof(*this) << std::endl;
    upto = (upto == 0) ? 64 : upto;
    int size = upto; //this->size();
    int stride = 4;
    if(size > stride) {
      int numrows = size/stride;
      for(int i=0; i<numrows; i++) {
        for(int j=0; j<stride; j++) {
          os << this->at(j+i*stride) << ' ';
        }
        os << "  |  ";
        for(int j=0; j<stride; j++) {
          os << reinterpret_cast<T*>(orig_)[j+i*stride] << ' ';
        }
        os << std::endl;
      }
    } else {
      for(int i=0; i<size; i++) {
        os << this->at(i) << '\t';
      }
    }

    os << std::endl;
    PackerEpilogue();
  }
//  virtual int Preserv(packer::CDPacker &packer) { CD_VECTOR_PRINT("CDVector Preserv \n"); return 0; }
//  virtual int Restore(packer::CDPacker &packer) { CD_VECTOR_PRINT("CDVector Restore \n"); return 0; }
  CDVector<T>(void) : name_(VECTOR_INIT_NAME) { Init(); }
  CDVector<T>(int len) : std::vector<T>(len), name_(VECTOR_INIT_NAME) { Init(); }
//  CDVector<T>(const std::initializer_list<T> &il) : std::vector<T>(il), name_(UNDEFINED_NAME) { Init(); }
  CDVector<T>(const std::initializer_list<T> &il) 
    : std::vector<T>(il), name_(VECTOR_INIT_NAME) { Init(); }
  CDVector<T>(const CDVector<T> &that) 
    : std::vector<T>(that), orig_(that.orig_), orig_size_(that.orig_size_), prv_size_(0), difference_(0.0), read_(false)
  {}
//  CDVector<T>(void) : name_(UNDEFINED_NAME) { Init(); }
//  CDVector<T>(int len, const char []str=UNDEFINED_NAME) : std::vector<T>(len), name_(str) { Init(); }
////  CDVector<T>(const std::initializer_list<T> &il) : std::vector<T>(il), name_(UNDEFINED_NAME) { Init(); }
//  CDVector<T>(const std::initializer_list<T> &il, const char []str=UNDEFINED_NAME) 
//    : std::vector<T>(il), name_(str) { Init(); }

  void Init(void) {
    orig_ = NULL;
    orig_size_ = 0;
    prv_size_ = 0;
    difference_ = 0.0;
    read_ = false;
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

  inline 
  void SetRead(long idx=0) { 
    //if(idx < 0 || idx > (long)this->size()) { printf("idx %ld >= size %zu\n", idx, this->size()); assert(0); }
    read_ = true; 
  }
  void CheckID(const std::string &str) {
    auto it = str2id.find(str);
    static int cnt = 0;
    if(it == str2id.end()) {
      str2id[str] = 0xffef50 + cnt++;
    }
    if(myTaskID == 0) {
    
      CD_VECTOR_PRINT("%s id:%s, %lu\n", __func__, str.c_str(), str2id[str]);
    }
  }
//  static inline float CompareVector(T *orip, T *newp, uint64_t size) {
//    if(size % 8 == 0) {
//      return Compare<uint64_t>(orip, newp, size);
//    } else if(size % 4 == 0) {
//      return Compare<uint32_t>(orip, newp, size);
//    } else {
//      return Compare<char *>(orip, newp, size);
//    }
//  }
//  template <typename T1>
//  float Compare(T1 *orip, T1 *newp, uint32_t size) {
//    float difference = 0.0;
//    uint32_t diff_cnt = 0;
//    uint32_t i=0;
//    if(size != 0) {
//      for(; i<size; i++, orip++, newp++) {
//        //printf("[%d] i:%u, size:%u, %p == %p\n", myTaskID, i, size, orip, newp);
//        diff_cnt = (*orip != *newp)? diff_cnt+1 : diff_cnt;
//      }
//      difference = (float)(diff_cnt) / size;
//    }
//    return difference;
//  }
};


}

#endif
