#include <iostream>
#include <vector>
#include <initializer_list>
#include <cstdlib>
#include <cstring>
using namespace std;
//using namespace packer;
#include "cd_packer.hpp"

class Interface {
  int size_;
  int offset_;
  int type_;
  public:
  virtual int Preserv(packer::CDPacker &packer) = 0;
  virtual int Restore(packer::CDPacker &packer) = 0;
};

template <typename T>
class MyVector : public vector<T> {
//class MyVector : public Interface {
  int orig_size;
  public:
//  virtual int Preserv(packer::CDPacker &packer) { printf("MyVector Preserve %zu %zu\n", this->size(), this->capacity()); return 0; }
//  virtual int Restore(packer::CDPacker &packer) { printf("MyVector Restore %zu %zu\n", this->size(), this->capacity()); return 0; }
  virtual int Preserv(packer::CDPacker &packer) { printf("MyVector Preserv \n"); return 0; }
  virtual int Restore(packer::CDPacker &packer) { printf("MyVector Restore \n"); return 0; }
  MyVector<T>(void) {}
  MyVector<T>(int len) : vector<T>(len) {}
  MyVector<T>(const initializer_list<T> &il) : vector<T>(il) {}
};

int main() {

  int arrA[8] = {1, 2, 3, 4, 5, 6, 7, 8}; // reference
  MyVector<int> vecA({1, 2, 3, 4, 5, 6, 7, 8});
//  vector<int> vecA({1, 2, 3, 4, 5, 6, 7, 8});
  cout << "Original ptr: " << vecA.data() << ", vec size:" << vecA.size() << ", vec cap: " << vecA.capacity() << ", vec obj size: " << sizeof(vecA) << endl;
  void *psave = malloc(4096);
  memcpy(psave, &vecA, sizeof(vecA));

  int count = 1;
  int i = 0;
  int last = vecA.size();
  cout << "Before "; for(int j=0; j<vecA.size(); j++) { cout << vecA[j] << ' '; }; cout << endl;
  for(; i<last; i++) {
    vecA[i] = i + count;
  } 
  last *= 2;
  for(; i<last; i++) {
    vecA.push_back(i + count);
  }
  cout << "After  "; for(int j=0; j<vecA.size(); j++) { cout << vecA[j] << ' '; }; cout << endl;

  cout << "Mutated  ptr: "<< vecA.data() << ", vec size:" << vecA.size() << ", vec cap: " << vecA.capacity() << ", vec obj size: " << sizeof(vecA) << endl;

//  memcpy(&vecA, psave, sizeof(vecA));
  vecA = *(MyVector<int> *)(psave);
//  vecA = *(vector<int> *)(psave);
  cout << "Restore  ptr: "<< vecA.data() << ", vec size:" << vecA.size() << ", vec cap: " << vecA.capacity() << ", vec obj size: " << sizeof(vecA) << endl;
  vecA.resize(4);
  cout << "Restore  ptr: "<< vecA.data() << ", vec size:" << vecA.size() << ", vec cap: " << vecA.capacity() << ", vec obj size: " << sizeof(vecA) << endl;

  vector< MyVector<int> * > vec_list;
  for(int j=0; j<4; j++) {
    vec_list.push_back(new MyVector<int>(8*(i+1)));
  }

  cout << "Before Preserve" << endl;
  packer::CDPacker mypacker;
  for(auto it=vec_list.begin(); it!=vec_list.end(); ++it) {
    (*it)->Preserv(mypacker);    
  }
  cout << "After Preserve" << endl;
  for(auto it=vec_list.begin(); it!=vec_list.end(); ++it) {
    (*it)->Restore(mypacker);    
  }

  cout << "After Restore" << endl;

  return 0;
}
