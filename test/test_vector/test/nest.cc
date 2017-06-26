#include <iostream>
#include <vector>
#include <initializer_list>
#include <cstdlib>
#include <cstring>
using namespace std;
//using namespace packer;
#include "cd_packer.hpp"
#include <cstdint>

class Interface {
protected:
  int size_;
  int offset_;
  uint64_t id_;
  static uint64_t gen_id;
public:
  Interface(void) : size_(0), offset_(0), id_(gen_id++) {}
  void SetID(uint64_t id) { id_ = id; }
  virtual int Preserv(packer::CDPacker &packer) = 0;
  virtual int Restore(packer::CDPacker &packer) = 0;
};

template <typename T>
class MyVector : public vector<T>, public Interface {
//class MyVector : public Interface {
  int orig_size;
  public:
  virtual int Preserv(packer::CDPacker &packer) { 

    printf("MyVector Preserve elemsize:%zu, %zu %zu\n", sizeof(T), this->size(), this->capacity()); 
    char *ptr = reinterpret_cast<char *>(this->data());
    packer.Add(ptr, packer::CDEntry(id_, this->size() * sizeof(T), 0, ptr));
    return 0; 
  }
  virtual int Restore(packer::CDPacker &packer) { 
    printf("MyVector Restore elemsize:%zu, %zu %zu\n", sizeof(T), this->size(), this->capacity()); 
    char *ptr = reinterpret_cast<char *>(this->data());
    packer::CDEntry *pentry = reinterpret_cast<packer::CDEntry *>(packer.Restore(id_, ptr, this->size() * sizeof(T)));
    this->resize(pentry->size() / sizeof(T));
    return 0; 
  }
  void Print(const char str[]="") {
    cout << str << " ptr: "<< this->data() 
                << ", vec size:" << this->size() 
                << ", vec cap: " << this->capacity() 
                << ", vec obj size: " << sizeof(*this) << endl;
    int size = this->size();
    int stride = 8;
    if(size > stride) {
      int numrows = size/stride;
      for(int i=0; i<numrows; i++) {
        for(int j=0; j<stride; j++) {
//          cout << "index: "<<  j + i*stride <<", "<< size/8 << endl;
          cout << this->at(j+i*stride) << '\t';
        }
        cout << endl;
      }
    } else {
      for(int i=0; i<size; i++) {
        cout << this->at(i) << '\t';
      }
    }

    cout << endl;
  }
//  virtual int Preserv(packer::CDPacker &packer) { printf("MyVector Preserv \n"); return 0; }
//  virtual int Restore(packer::CDPacker &packer) { printf("MyVector Restore \n"); return 0; }
  MyVector<T>(void) {}
  MyVector<T>(int len) : vector<T>(len) {}
  MyVector<T>(const initializer_list<T> &il) : vector<T>(il) {}
};
static int count = 1;
template <typename T>
void MutateVector(T &vec) {
  int i=0;
  int last = vec.size();
  for(; i<last; i++) {
    vec[i] += count;
  }
  auto last_elem = vec[last-1];
  last *= 2;
  for(; i<last; i++) {
    vec.push_back(last_elem + count + i);
  }

  count++;
}
uint64_t Interface::gen_id = 0;

int main() {
  packer::CDPacker packer;
  int arrA[8] = {0, 1, 2, 3, 4, 5, 6, 7}; // reference
  MyVector<int> vecA({0, 1, 2, 3, 4, 5, 6, 7});
  MyVector<float> vecB({0.0, 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7});
//  vector<int> vecA({1, 2, 3, 4, 5, 6, 7, 8});
  cout << "Original ptr: " << vecA.data() << ", vec size:" << vecA.size() << ", vec cap: " << vecA.capacity() << ", vec obj size: " << sizeof(vecA) << endl;
//  void *psave = malloc(4096);
//  memcpy(psave, &vecA, sizeof(vecA));

  vecA.Preserv(packer);
  vecB.Preserv(packer);
  vecA.Print("[Before]"); 
  vecB.Print("[Before]"); 
  MutateVector(vecA);
  MutateVector(vecB);
  vecA.Print("[Mutated]"); 
  vecB.Print("[Mutated]"); 
//  cout << "After  "; for(int j=0; j<vecA.size(); j++) { cout << vecA[j] << ' '; }; cout << endl;
//
//  cout << "Mutated  ptr: "<< vecA.data() << ", vec size:" << vecA.size() << ", vec cap: " << vecA.capacity() << ", vec obj size: " << sizeof(vecA) << endl;
  vecA.Restore(packer);
  vecB.Restore(packer);
//  memcpy(&vecA, psave, sizeof(vecA));
//  vecA = *(MyVector<int> *)(psave);
//  vecA = *(vector<int> *)(psave);
//  cout << "Restore  ptr: "<< vecA.data() << ", vec size:" << vecA.size() << ", vec cap: " << vecA.capacity() << ", vec obj size: " << sizeof(vecA) << endl;
  vecA.Print("[Restore]"); 
  vecB.Print("[Restore]"); 
//  vecA.resize(4);
//  cout << "Restore  ptr: "<< vecA.data() << ", vec size:" << vecA.size() << ", vec cap: " << vecA.capacity() << ", vec obj size: " << sizeof(vecA) << endl;
//
//  vector< Interface * > vec_list;
//  for(int j=0; j<4; j++) {
//    auto mvec = new MyVector<int>(8*(j+1));
//    vec_list.push_back(mvec);
//    printf("new vec size:%zu %zu\n", mvec->size(), mvec->capacity());
//  }
//
//  cout << "Before Preserve" << endl;
//  packer::CDPacker mypacker;
//  for(auto it=vec_list.begin(); it!=vec_list.end(); ++it) {
//    (*it)->Preserv(mypacker);    
//  }
//  cout << "After Preserve" << endl;
//  for(auto it=vec_list.begin(); it!=vec_list.end(); ++it) {
//    (*it)->Restore(mypacker);    
//  }
//
//  cout << "After Restore" << endl;

  return 0;
}
