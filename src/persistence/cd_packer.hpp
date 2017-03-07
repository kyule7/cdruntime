#include "packer.hpp"

namespace packer {
//  Preservation
//    1. Single entry
//    2. 
// Restoration
//    1. Prefetch
class CDPacker : public Packer<CDEntry> {
  public:
    CDPacker(TableStore<CDEntry> *table=NULL, DataStore *data=NULL) 
      : Packer<CDEntry>(table, data) {}
    CDPacker(bool alloc, TableStore<CDEntry> *table, DataStore *data=NULL) 
      : Packer<CDEntry>(alloc, table, data) {}
    char *Restore(uint64_t tag, char *dst=NULL) 
    {
      void *ret = dst;
      MYDBG("tag:%lu\n", tag);
      // 1. Find entry in table store
      // 2. Copy data from data store to src
      CDEntry *pentry = reinterpret_cast<CDEntry *>(table_->Find(tag));
      if(pentry == NULL) {
        for(int i=0; i<table_->used(); i++) {
          printf("table:%lu\n", (*table_)[i].id_);
        }
      }
      PACKER_ASSERT(pentry != NULL);
      CDEntry entry = *pentry;
      dst = (dst == NULL)? entry.src_ : dst;
      PACKER_ASSERT(dst == entry.src_);
      if( entry.size_.Check(Attr::knested) == false ) {
        data_->Read(dst, entry.size(), entry.offset_);
      } else {
        // Table format
        // [ID] [SIZE]  [OFFSET] [SRC]
        //  ID  totsize  offset  tableoffset
        posix_memalign(&ret, CHUNK_ALIGNMENT, entry.size());
        data_->Read((char *)ret, entry.size(), entry.offset_);
        MagicStore *magic = reinterpret_cast<MagicStore *>(ret);
        int *packed_data = reinterpret_cast<int *>((char *)ret + sizeof(MagicStore));
        for(int i=0; i<1024/64; i++) {
          for(int j=0; j<16; j++) {
            printf("%4d ", *((int *)ret + i*16 + j));
          }
          printf("\n");
        } 
        printf("Read id:%lu, attr:%lx chunk: %lu at %lu, "
               "Read MagicStore: size:%lu, tableoffset:%lu, entry_type:%u\n", 
                entry.id_, entry.attr(), entry.size(), entry.offset(), 
                magic->total_size_, magic->table_offset_, magic->entry_type_);
        getchar();
//        PACKER_ASSERT(magic->table_offset_ == (uint64_t)entry.src_);
        PACKER_ASSERT_STR(magic->total_size_ == entry.size(), 
            "magic size:%lu==%lu\n", magic->total_size_, entry.size());
        MYDBG("Read MagicStore: size:%lu, tableoffset:%lu, entry_type:%u\n", 
              magic->total_size_, magic->table_offset_, magic->entry_type_);
      }
      return (char *)ret;
    }
};


}
