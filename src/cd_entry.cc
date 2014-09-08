#include "cd_entry.h"

using namespace cd;


CDEntry::CDEntryErrT CDEntry::Delete(void)
{
	return CDEntry::CDEntryErrT::kOK;
}


//GONG
CDEntry::CDEntryErrT CDEntry::Restore(bool open, struct tsn_log_struct *log)
//#else
//enum CDEntry::CDEntryErrT CDEntry::Restore()
//#endif
{
  DataHandle *dst_data_temp=0;
  DataHandle real_dst_data=0; 

	if( dst_data_.handle_type() == DataHandle::kReference) 
	{
    dst_data_temp =&real_dst_data;

		cd::CDHandle* parent_cd; // FIXME: for now let's just search immediate parent only.  Let's extend this to more general way.  
		if( ptr_cd_ == 0 ) ERROR_MESSAGE("Pointer to CD object is not set.");
		parent_cd = GetParentCD(ptr_cd_->GetCDID());

		CDEntry* entry = parent_cd->ptr_cd()->InternalGetEntry(dst_data_.ref_name());
		if( entry != 0 ) {
//GONG

//Here, we search Parent's entry and Parent's Parent's entry and so on.
//if ref_name does not exit, we believe it's original. 
//Otherwise, there is original copy somewhere else, maybe grand parent has it. 
			cd::CD *p_cd;
			while(!entry->dst_data_.ref_name().empty()){

				std::cout<<entry->dst_data_.ref_name()<<std::endl;
		  	p_cd = parent_cd->ptr_cd();	 

				std::cout<<p_cd->GetCDID().level_<<std::endl;
				parent_cd = GetParentCD(p_cd->GetCDID());
				entry = p_cd->InternalGetEntry(entry->dst_data_.ref_name());
//		  	cd::CD *p_cd_ = gp_cd.ptr_cd();	
//				cout<<p_cd_->cd_id().level_<<endl;
			}
			lsn = entry->lsn;

			
      *dst_data_temp = entry->dst_data_; 
      dst_data_temp->set_ref_offset(dst_data_.ref_offset() );    
      dst_data_temp->set_len (dst_data_.len());     // length could be different in case we want to describe only part of the preserved entry.
		}
    else 
    {
      ERROR_MESSAGE("Failed to retrieve a CDEntry");
    }
		//FIXME: Assume the destination (it is a source in restoration context) is local. Basically parent's entry's destination is also local. So that we can acheive this by just copying memory or reading file   

	}
  else // if it is not via reference just use the current dst_data_
  {
//cout<<"kCopy in Restore()"<<endl;
    dst_data_temp = &dst_data_;
  }


	//FIXME we need to distinguish whether this request is on Remote or local for both when using kOSFile or kMemory and do appropriate operations..
	if(dst_data_temp->handle_type() == DataHandle::kMemory)
	{

		if(src_data_.address_data() != NULL) 
		{
			memcpy(src_data_.address_data(), (char *)dst_data_temp->address_data()+dst_data_temp->ref_offset(), dst_data_temp->len()); 
		}
		else 
		{
			ERROR_MESSAGE("Not enough memory.");
		}
	}
	else if( dst_data_temp->handle_type() == DataHandle::kOSFile) 
	{
    //GONG
    std::cout<<"kOSFile file_name() : "<<dst_data_temp->file_name()<<" offset: "<<dst_data_temp->ref_offset()<<std::endl;					
		if(!open){
			int ret;
//			ret = tsn_log_create_file(dst_data_temp->file_name());
//			assert(ret>=0);	
			ret = tsn_log_open_file(dst_data_temp->file_name(), TSN_LOG_READWRITE, log);
			assert(ret>=0);	
		}
    assert(log);
    assert(log->log_ops);
		uint64_t bytes = log->log_ops->l_read(log, &lsn, src_data_.address_data(), src_data_.len());
		std::cout<<lsn.lsn<<" "<<bytes<<" "<<open<<std::endl;
		assert(bytes == src_data_.len());
//#else
//		//FIXME we need to collect file writes and write as a chunk. We don't want to have too many files per one CD.   
//
//		FILE *fp;
//		char *cd_file; 
//		//  Util::GetUniqueCDFileName(cd_id_, src_data_.address_data(), cd_file); // assume cd_id_ is unique (sequential cds will have different cd_id) and address data is also unique by natural
//		cd_file = dst_data_temp->file_name();
//		fp = fopen(cd_file, "r");
//		if( fp!= NULL )
//		{
//      if( dst_data_temp->ref_offset() != 0 )
//        fseek(fp, dst_data_temp->ref_offset(), SEEK_SET); 
//			fread(src_data_.address_data(), 1, src_data_.len(), fp);
//			fclose(fp);
//			return kOK;
//		}
//		else return kFileOpenError;
//#endif

	}


	return CDEntry::CDEntryErrT::kOK;
	//Direction is from dst_data_ to src_data_

}
