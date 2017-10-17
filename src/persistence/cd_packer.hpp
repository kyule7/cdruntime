#include "packer.hpp"
#define CD_PACKER_PRINT(...)

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
    CDPacker(TableStore<CDEntry> *table, uint64_t table_size, DataStore *data, uint64_t data_size=DATA_GROW_UNIT) 
      : Packer<CDEntry>(table_size, table, data, data_size, DEFAULT_FILEMODE) {}
    CDPacker(bool alloc, TableStore<CDEntry> *table, DataStore *data=NULL) 
      : Packer<CDEntry>(alloc, table, data) {}
    virtual ~CDPacker() {}
    CDEntry *Restore(uint64_t tag, char *dst=NULL, uint64_t len=0) 
    {
//      void *ret = dst;
      MYDBG("tag:%lu\n", tag);
      CD_PACKER_PRINT("tag:%lu\n", tag); //getchar();
      // 1. Find entry in table store
      // 2. Copy data from data store to src
      //CDEntry *pentry = reinterpret_cast<CDEntry *>(table_->Find(tag));
      CDEntry *pentry = table_->Find(tag);
//      CDEntry *ret = pentry;
      int found_result = -1;      
      if(pentry == NULL) {
        printf("[%d] not found %lu\n", packerTaskID, tag);
        //return NULL;
        found_result = 0;
      } else if(pentry->src_ == NULL) {
        if(packerTaskID == 0) {
        printf("[%d] previously null %lu utr:%p, size:%lu, len:%lu, offset:%lx\n", 
            packerTaskID, tag, pentry->src_, pentry->size(), len, pentry->offset_);
        }
        // when preserved, data was null.
//        return pentry;
        found_result = 1;
      } else if(pentry->size() == 0) {
        if(packerTaskID == 0) {
        printf("[%d] %lu dst:%p previously size:%lu, requested len:%lu\n", 
            packerTaskID, tag, pentry->src_, pentry->size(), len);
        }
//        return pentry;
        found_result = 2;
      } else if(len > pentry->size()) {
        printf("[%d] %lu dst:%p reallocated from size:%lu to %lu (requested len)\n", 
            packerTaskID, tag, pentry->src_, pentry->size(), len);
        found_result = 3;
      } else {
        if(packerTaskID == 0) {
        printf("[Restore! %d] Preserved %lu ptr:%p, size:%lu, len:%lu, offset:%lx\n", 
            packerTaskID, tag, pentry->src_, pentry->size(), len, pentry->offset_);
        }
        found_result = 4;
      }
      switch(found_result) {
        case 0:
        case 1:
        case 2: {
//          printf("return:%d\n", found_result);
          return pentry;
                }
        case 3:
        case 4:
          break;
        default:
          break;

      }
      CD_PACKER_PRINT("next tag:%lu\n", tag); //getchar();
//      if(pentry == NULL) {
//        for(int i=0; i<table_->used(); i++) {
//          CD_PACKER_PRINT("table:%lu\n", (*table_)[i].id_);
//        }
//      }
      PACKER_ASSERT(pentry != NULL);
      CDEntry entry = *pentry;
      dst = (dst == NULL)? entry.src_ : dst;
//      PACKER_ASSERT(dst == entry.src_);

#if 1
      if( entry.size_.Check(Attr::knested) == false) {
        CD_PACKER_PRINT("no nested\n");
        data_->GetData(dst, entry.size(), entry.offset_);
        //getchar();
      } else {
        CD_PACKER_PRINT("nested\n");
        //getchar();
//        uint64_t produced = data_->Fetch(entry.size(), entry.offset_);
//        CD_PACKER_PRINT("\n\n TEST $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$4\n"); //getchar();
        uint64_t table_offset = (uint64_t)entry.src_;
        uint64_t data_size   = table_offset - entry.offset_; //entry.offset_ - table_offset;
        uint64_t table_size  = entry.size() - data_size;
#ifdef _DEBUG_0402        
        if(packerTaskID == 0) {
          CD_PACKER_PRINT("#######################################################\n"
                 "Check entry for packed obj:totsize:%lx offsetFromMagic:%lx, tablesize:%lx, tableoffset:%lx \n"
                 "#######################################################\n", 
            entry.size(), entry.offset_, table_size, table_offset); //getchar();
        }
#endif
        data_->Fetch(table_size, table_offset);
        CDEntry *pEntry_check = reinterpret_cast<CDEntry *>(
            data_->GetPtr(table_offset + data_->head()));
        void *tmp_ptr = NULL;
        if(posix_memalign(&tmp_ptr, CHUNK_ALIGNMENT, table_size) != 0) {
          assert(0);
        }
        CDEntry *pEntry = (CDEntry  *)tmp_ptr;
        memcpy(pEntry, pEntry_check, table_size);
        // Test
#ifdef _DEBUG_0402        
        if(packerTaskID == 0)
        {
          CD_PACKER_PRINT("\n\n $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$4\n");
          uint64_t *ptest = reinterpret_cast<uint64_t *>(pEntry_check);
          for(uint64_t i=0; i<table_size/sizeof(CDEntry); i++) {
            CD_PACKER_PRINT("%4lx %8lx %8lx %8lx\n", *ptest, *(ptest+1), *(ptest+2), *(ptest+3));
            ptest += 4;
          }
          CD_PACKER_PRINT("\n$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$4\n");
          ptest = reinterpret_cast<uint64_t *>(pEntry);
          for(uint64_t i=0; i<table_size/sizeof(CDEntry); i++) {
            CD_PACKER_PRINT("%4lx %8lx %8lx %8lx\n", *ptest, *(ptest+1), *(ptest+2), *(ptest+3));
            ptest += 4;
          }
          CD_PACKER_PRINT("\n\n $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$4\n\n\n");
          //getchar();
        }
#endif
        // This will just read from cache (memory)
        uint64_t num_entries = table_size/sizeof(CDEntry);
//        CD_PACKER_PRINT("# of entries:%lu, head:%lu, pos_buf:%lu\n", num_entries, data_->head(), pEntry->offset_); //getchar();
        for(uint32_t i=0; i<num_entries; i++, pEntry += 1) {
#ifdef _DEBUG_0402        
          if(packerTaskID == 0) {
            CD_PACKER_PRINT("[%u/%lu] id:%lx Restore: %p size:%lx, offset:%lx\n", 
                i, num_entries, pEntry->id_,  pEntry->src_, pEntry->size(), pEntry->offset_);
            uint64_t *ptest = (uint64_t *)pEntry;
            CD_PACKER_PRINT("%4lx %8lx %8lx %8lx\n", *ptest, *(ptest+1), *(ptest+2), *(ptest+3));
          }
#endif
          data_->GetData(pEntry->src_, pEntry->size(), pEntry->offset_);
        }
        free(tmp_ptr);
//        PACKER_ASSERT_STR(align_up(table_size) == consumed, 
//            "tablesize:%lu==%lu(consumed)\n", table_size, consumed);

      }
#else
      if( entry.size_.Check(Attr::knested) == false ) {
        data_->Read(dst, entry.size(), entry.offset_);
      } else {
        // Table format
        // [ID] [SIZE]  [OFFSET] [SRC]
        //  ID  totsize  offset  tableoffset
        posix_memalign(&ret, CHUNK_ALIGNMENT, entry.size());
        CD_PACKER_PRINT("Read:%lu, %lu\n", entry.size(), entry.offset_);
        data_->Read((char *)ret, entry.size(), entry.offset_);
        MagicStore *magic = reinterpret_cast<MagicStore *>(ret);
        //uint64_t *packed_data = reinterpret_cast<uint64_t *>((char *)ret + sizeof(MagicStore));
        int *packed_data = reinterpret_cast<int *>(ret);
        CD_PACKER_PRINT("#### check read magicstore###\n");
        for(int i=0; i<128/16; i++) {
          for(int j=0; j<16; j++) {
            CD_PACKER_PRINT("%4d ", *(packed_data + i*16 + j));
          }
          CD_PACKER_PRINT("\n");
        } 
        CD_PACKER_PRINT("#### check read magicstore###\n");
        CD_PACKER_PRINT("Read id:%lu, attr:%lx chunk: %lu at %lu, "
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
      return pentry;
    }
};


}
