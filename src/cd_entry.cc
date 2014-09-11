#include "cd_entry.h"
#include <cassert>
using namespace cd;


CDEntry::CDEntryErrT CDEntry::Delete(void)
{
	return CDEntry::CDEntryErrT::kOK;
}



CDEntry::CDEntryErrT CDEntry::SaveMem(void)
{
  if(dst_data_.address_data() == NULL) {
    void *allocated_space;
    allocated_space = DATA_MALLOC(dst_data_.len() * sizeof(char));  
    // FIXME: Jinsuk we might want our own memory manager since we don't want to call malloc everytime we want small amount of additional memory. 
    dst_data_.set_address_data(allocated_space);
  }
  else {

    memcpy(dst_data_.address_data(), src_data_.address_data(), src_data_.len());
    if(false) { // Is there a way to check if memcpy is done well?
      ERROR_MESSAGE("Not enough memory.");
      assert(0);
      return kOutOfMemory; 
    }

  }

  return kOK;
}

CDEntry::CDEntryErrT CDEntry::SaveFile(std::string base_, bool open, struct tsn_log_struct *log)
{
  // Get file name to write if it is currently NULL
  if( dst_data_.file_name() == NULL ) {
    // assume cd_id_ is unique (sequential cds will have different cd_id) 
    // and address data is also unique by natural
//      char *cd_file = new char[MAX_FILE_PATH];
//      cd::Util::GetUniqueCDFileName(cd_id_, src_data_.address_data(), cd_file, base_); 
    dst_data_.set_file_name(cd::Util::GetUniqueCDFileName(cd_id_, src_data_.address_data(), base_.c_str()));
  }

 	if(!open) {
		int ret;
    printf("inside saveMem\n");
    getchar();
		ret = tsn_log_create_file(dst_data_.file_name());
	  assert(ret>=0);	
		ret = tsn_log_open_file(dst_data_.file_name(), TSN_LOG_READWRITE, log);
	  assert(ret>=0);	
	}
  assert(log);
  assert(log->log_ops);
	uint64_t bytes = log->log_ops->l_write(log, src_data_.address_data(), src_data_.len(), &lsn);
	std::cout<<lsn.lsn<<std::endl;
	assert(bytes == src_data_.len());
//		int err = log->log_ops->l_flush(log, &lsn, 1, &durable_lsn);
//		assert(err);

	return kOK;
	
}

CDEntry::CDEntryErrT CDEntry::Save(void)
{
  //Direction is from src_data_ to dst_data_

  //FIXME Jinsuk: Just for now we assume everything works on memory
  // What we need to do is that distinguish the medium type and then do operations appropriately

  //FIXME we need to distinguish whether this request is on Remote or local for both when using kOSFile or kMemory and do appropriate operations..
  //FIXME we need to distinguish whether this is to a reference....
  if( dst_data_.handle_type() == DataHandle::kMemory ) {
    if(dst_data_.address_data() == NULL) {

      void *allocated_space;

      // FIXME: Jinsuk we might want our own memory manager since we don't want to call malloc everytime we want small amount of additional memory. 
      allocated_space = DATA_MALLOC(dst_data_.len() * sizeof(char));  

      dst_data_.set_address_data(allocated_space);

    }

    if(dst_data_.address_data() != NULL) {
      memcpy(dst_data_.address_data(), src_data_.address_data(), src_data_.len()); 
    }
    else {
      return kOutOfMemory;
      //  ERROR_MESSAGE("Not enough memory.");
    }

  }
  else if( dst_data_.handle_type() == DataHandle::kOSFile ) {
    if( dst_data_.file_name() == NULL ) {
      char *cd_file = new char[MAX_FILE_PATH];
      cd::Util::GetUniqueCDFileName(cd_id_, src_data_.address_data(), cd_file); 
      // assume cd_id_ is unique (sequential cds will have different cd_id) and address data is also unique by natural
      dst_data_.set_file_name(cd_file);
    }

    // FIXME	Jinsuk: we need to collect file writes and write as a chunk. 
    // We don't want to have too many files per one CD. So perhaps one file per chunk is okay.

    FILE *fp;
    //char filepath[1024]; 
    //Util::GetCDFilePath(cd_id_, src_data.address_data(), filepath); 
    // assume cd_id_ is unique (sequential cds will have different cd_id) and address data is also unique by natural
    fp = fopen(dst_data_.file_name(), "w");
    if( fp!= NULL ) {
      fwrite(src_data_.address_data(), sizeof(char), src_data_.len(), fp);
      fclose(fp);
      return kOK;
    }
    else return kFileOpenError;
  }


  return kOK;

}


void CDEntry::CloseFile(struct tsn_log_struct *log)
{
	int ret;
	ret = tsn_log_close_file(log);
	assert(ret>=0);
	ret = tsn_log_destroy_file(dst_data_.file_name());	
}

DataHandle* CDEntry::GetBuffer() {
  DataHandle* buffer=0;
  DataHandle real_dst_data;
	if( dst_data_.handle_type() == DataHandle::kReference) {  // Restoration from reference
    buffer = &real_dst_data;
    
    // FIXME: for now let's just search immediate parent only.  Let's extend this to more general way.
		cd::CDHandle* parent_cd = GetParentCD(ptr_cd_->GetCDID());
		if( ptr_cd_ == 0 ) { ERROR_MESSAGE("Pointer to CD object is not set."); assert(0); }

		CDEntry* entry = parent_cd->ptr_cd()->InternalGetEntry(dst_data_.ref_name());

		if( entry != 0 ) {

      //Here, we search Parent's entry and Parent's Parent's entry and so on.
      //if ref_name does not exit, we believe it's original. 
      //Otherwise, there is original copy somewhere else, maybe grand parent has it. 

			cd::CD* p_cd = parent_cd->ptr_cd();
			do {

				std::cout <<"ref name: "    << entry->dst_data_.ref_name() 
                  << ", at level: " << p_cd->GetCDID().level_<<std::endl;

				entry = p_cd->InternalGetEntry(entry->dst_data_.ref_name());
        if(entry == 0) assert(0);

				parent_cd = GetParentCD(p_cd->GetCDID());
        if(parent_cd == 0) assert(0);

			} while( !entry->dst_data_.ref_name().empty() );

			lsn = entry->lsn;
			
      *buffer = entry->dst_data_; 
      buffer->set_ref_offset(dst_data_.ref_offset() );   
      // length could be different in case we want to describe only part of the preserved entry.
      buffer->set_len (dst_data_.len());     
		}
    else {  // no entry!!
      ERROR_MESSAGE("Failed to retrieve a CDEntry");
    }
		//FIXME: Assume the destination (it is a source in restoration context) is local. 
    // Basically parent's entry's destination is also local. So that we can acheive this by just copying memory or reading file   

	}
  else {  // Restoration from memory or file system. Just use the current dst_data_ for it.
    std::cout<<"kCopy in Restore()"<<std::endl;
    buffer = &dst_data_;
  }
}

//GONG
CDEntry::CDEntryErrT CDEntry::Restore(bool open, struct tsn_log_struct *log)
{
  // Populate buffer
  DataHandle* buffer = GetBuffer();
  if(buffer == 0) { ERROR_MESSAGE("GetBuffer failed.\n"); assert(0); }
	//FIXME we need to distinguish whether this request is on Remote or local for both when using kOSFile or kMemory and do appropriate operations..
	if(buffer->handle_type() == DataHandle::kMemory) {
    assert(buffer!=NULL);
    std::cout<<buffer<<" " <<buffer->address_data() << " "<< buffer->ref_offset() << " "<< buffer->len()<<std::endl;
		if(src_data_.address_data() != NULL) {
			memcpy(src_data_.address_data(), 
            (char*)buffer->address_data() + (buffer->ref_offset()), buffer->len()); 
		}
		else {
			ERROR_MESSAGE("Not enough memory.");
		}

	}
	else if( buffer->handle_type() == DataHandle::kOSFile) {
// FIXME: GONG
//    std::cout<<"kOSFile file_name() : "<<buffer->file_name()<<" offset: "<<buffer->ref_offset()<<std::endl;					
//		if(!open){
//			int ret;
//			ret = tsn_log_create_file(buffer->file_name());
//			assert(ret>=0);	
//			ret = tsn_log_open_file(buffer->file_name(), TSN_LOG_READWRITE, log);
//			assert(ret>=0);	
//		}
//    assert(log);
//    assert(log->log_ops);
//		uint64_t bytes = log->log_ops->l_read(log, &lsn, src_data_.address_data(), src_data_.len());
//		std::cout<<lsn.lsn<<" "<<bytes<<" "<<open<<std::endl;
//		assert(bytes == src_data_.len());



		//FIXME we need to collect file writes and write as a chunk. We don't want to have too many files per one CD.   

		FILE *fp;
		char *cd_file; 
		//  Util::GetUniqueCDFileName(cd_id_, src_data_.address_data(), cd_file); // assume cd_id_ is unique (sequential cds will have different cd_id) and address data is also unique by natural
		cd_file = buffer->file_name();
		fp = fopen(cd_file, "r");
		if( fp!= NULL ) {
      if( buffer->ref_offset() != 0 )
        fseek(fp, buffer->ref_offset(), SEEK_SET); 
			fread(src_data_.address_data(), 1, src_data_.len(), fp);
			fclose(fp);
			return kOK;
		}
		else return kFileOpenError;

	}


	return CDEntry::CDEntryErrT::kOK;
	//Direction is from dst_data_ to src_data_

}
