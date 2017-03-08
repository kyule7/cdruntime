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

#if 1
      if( entry.size_.Check(Attr::knested) == false) {
        data_->Read(dst, entry.size(), entry.offset_);
      } else {
        
        data_->ForwardFetch(entry.size(), entry.offset_);
        uint64_t table_size = entry.offset_ - (uint64_t)entry.src_;
        CDEntry *pEntry = reinterpret_cast<CDEntry *>(
            data_->ForwardFetchRead(table_size, (uint64_t)entry.src_) );
        {
          printf("\n\n $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$4\n");
          uint64_t *pEntry = reinterpret_cast<uint64_t *>(ret);
          for(uint64_t i=0; i<table_size/sizeof(CDEntry); i++) {
            printf("%6lu %6lu %6lu %6lu\n", *pEntry, *(pEntry+1), *(pEntry+2), *(pEntry+3));
            pEntry += 4;
          }
          printf("\n\n $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$4\n\n\n");
          getchar();
        }
        // This will just read from cache (memory)
        for(uint32_t i=0; i<table_size/sizeof(CDEntry); i++) {
          data_->Read(pEntry->src_, pEntry->size(), pEntry->offset_);
        }
      }
#else
      if( entry.size_.Check(Attr::knested) == false ) {
        data_->Read(dst, entry.size(), entry.offset_);
      } else {
        // Table format
        // [ID] [SIZE]  [OFFSET] [SRC]
        //  ID  totsize  offset  tableoffset
        posix_memalign(&ret, CHUNK_ALIGNMENT, entry.size());
        printf("Read:%lu, %lu\n", entry.size(), entry.offset_);
        data_->Read((char *)ret, entry.size(), entry.offset_);
        MagicStore *magic = reinterpret_cast<MagicStore *>(ret);
        //uint64_t *packed_data = reinterpret_cast<uint64_t *>((char *)ret + sizeof(MagicStore));
        int *packed_data = reinterpret_cast<int *>(ret);
        printf("#### check read magicstore###\n");
        for(int i=0; i<128/16; i++) {
          for(int j=0; j<16; j++) {
            printf("%4d ", *(packed_data + i*16 + j));
          }
          printf("\n");
        } 
        printf("#### check read magicstore###\n");
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
#endif
      return (char *)ret;
    }
};


}
