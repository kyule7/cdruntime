#include "packer.hpp"

namespace cd {

// data packer
class CDPacker : public Packer<CDEntry> {
  //Packer(uint64_t alloc=1, TableStore<EntryT> *table=NULL, DataStore *data=NULL) {
  CDPacker(void) : Packer(1, new TableStore<CDEntry>, new FileDataStore) {}
  
  
};

}
