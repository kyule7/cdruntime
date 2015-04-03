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

#include "cd_entry.h"
#include "cd_path.h"

using namespace cd;
using namespace std;

CDEntry::CDEntryErrT CDEntry::Delete(void)
{
  // Do something

  if( dst_data_.address_data() != NULL ) {
    DATA_FREE( dst_data_.address_data() );
    dbg << "free preserved data\n" << endl; 
  }

  if( dst_data_.handle_type() == DataHandle::kOSFile )  {

#if _PRV_FILE_NOT_ERASED
    char cmd[30];
    char backup_dir[30];
    sprintf(backup_dir, "mkdir backup_results");
    int ret = system(backup_dir);
    sprintf(cmd, "mv %s ./backup_results/", dst_data_.file_name_);
    ret = system(cmd);
#else
    char cmd[30];
    sprintf(cmd, "rm %s", dst_data_.file_name_);
    int ret = system(cmd);
#endif

    if( ret == -1 ) {
      cerr << "Failed to create a directory for preservation data (SSD)" << endl;
      assert(0);
    }
  }

	return CDEntry::CDEntryErrT::kOK;
}



CDEntry::CDEntryErrT CDEntry::SaveMem(void)
{
  bool app_side_temp = app_side;
  app_side = false;

//  dbg<< "SaveMem(void) this: " << this<<endl;
  if(dst_data_.address_data() == NULL) {
    void *allocated_space = DATA_MALLOC(dst_data_.len() * sizeof(char));
    // FIXME: Jinsuk we might want our own memory manager 
    // since we don't want to call malloc everytime we want small amount of additional memory. 
    dst_data_.set_address_data(allocated_space);
//    dbg <<"..." << dst_data_.len()*sizeof(char) << endl;;
//    dbg << "allocated memory: " << allocated_space << endl;
//    dbg << "dst_data_: " << dst_data_.address_data() << endl;
//    dbgBreak();
  }
  memcpy(dst_data_.address_data(), src_data_.address_data(), src_data_.len());

  dbg << "\tSaved Data "<<tag2str[entry_tag_]<<" : " << *(reinterpret_cast<int*>(dst_data_.address_data())) 
      << " at " << dst_data_.address_data() <<" (Memory)" << endl; 
//  dbg<<"memcpy "<< src_data_.len() <<" size data from "<< src_data_.address_data() << " to "<< dst_data_.address_data() <<endl<<endl;

  if(false) { // Is there a way to check if memcpy is done well?
    cerr << "Not enough memory." << endl;
    assert(0);
    app_side = app_side_temp;
    return kOutOfMemory; 
  }


  app_side = app_side_temp;
  return kOK;
}

CDEntry::CDEntryErrT CDEntry::SaveFile(string base_, bool isOpen)
{
  // Get file name to write if it is currently NULL
  char* str; 
  if( !strcmp(dst_data_.file_name_, INIT_FILE_PATH) ) {
    // assume cd_id_ is unique (sequential cds will have different cd_id) 
    // and address data is also unique by natural
//      char *cd_file = new char[MAX_FILE_PATH];
//      cd::Util::GetUniqueCDFileName(cd_id_, src_data_.address_data(), cd_file, base_); 
    const char* data_name;
    str = new char[30];
    if(entry_tag_ == 0) {
      sprintf(str, "%d", rand());
      data_name = str;
    }
    else  {
      data_name = tag2str[entry_tag_].c_str();
    }

    const char *file_name = Util::GetUniqueCDFileName(ptr_cd_->GetCDID(), base_.c_str(), data_name).c_str();
    strcpy(dst_data_.file_name_, file_name); 
  }

  FILE *fp = fopen(dst_data_.file_name_, "w");
  if( fp!= NULL ) {
    fwrite(src_data_.address_data(), sizeof(char), src_data_.len(), fp);
    fclose(fp);

    dbg << "we write a file \n" << endl;

    return kOK;
  }
  else return kFileOpenError;
	
}




CDEntry::CDEntryErrT CDEntry::SavePFS(void)
{
	//First we should check for PFS file => I think we have checked it before calling this function (not sure).
	//PMPI_Status preserve_status;//This variable can be used to non-blocking writes to PFS. By checking this variable we can understand whether write has been finished or not.
  //PMPI_File_get_position( ptr_cd_->PFS_d_, &(dst_data_.parallel_file_offset_));

  // Dynamic Chunk Interleave
  dst_data_.parallel_file_offset_ = ptr_cd_->pfs_handler_->Write( src_data_.address_data(), src_data_.len() );

	return kOK;
}


// -----------------------------------------------------------------------------------------------
void CDEntry::RequestEntrySearch(void)
{
#if _MPI_VER
  dbg << "\nCDEntry::RequestEntrySearch\n" << endl;
  dbg.flush();
  
  CDHandle *curr_cdh = CDPath::GetCDLevel(ptr_cd_->level());
  curr_cdh = CDPath::GetCoarseCD(curr_cdh);
  dbg << "SIZE :" << curr_cdh->task_size() << ", LEVEL : " << curr_cdh->level() << " from " << ptr_cd_->level() << endl;
  CD *curr_cd = curr_cdh->ptr_cd();
  // SetMailbox
  CDEventT entry_search = kEntrySearch;
  dbg.flush();
  curr_cd->SetMailBox(entry_search);

  dbg << "SIZE :" << curr_cdh->task_size() << ", LEVEL : " << curr_cdh->level() << " from " << ptr_cd_->level() << endl;

  ENTRY_TAG_T tag_to_search = dst_data_.ref_name_tag();
  // Tag should contain CDID+entry_tag
  dbg << "[ToHead] CHECK TAG : " << curr_cd->cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, curr_cd->task_in_color())
      << " (Entry Tag: " << tag_to_search << ")" << endl;

  ENTRY_TAG_T sendBuf[2] = {tag_to_search, static_cast<ENTRY_TAG_T>(myTaskID)};
  dbg << "sendBuf[0] : " << sendBuf[0] << ", sendBuf[1] : " << sendBuf[1] << endl;
  dbg << "CHECK IT OUT : " << curr_cd->task_in_color() << " --> "<< curr_cd->head() << endl;
  dbg.flush();
  curr_cd->entry_request_req_[tag_to_search] = CommInfo();
//  curr_cd->entry_req_.push_back(CommInfo()); getchar();
  PMPI_Isend(sendBuf, 
             2, 
             MPI_UNSIGNED_LONG_LONG,
             curr_cd->head(), 
             curr_cd->cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, curr_cd->task_in_color()),
//             11,
//             MSG_TAG_ENTRY_TAG,
             curr_cd->color(),
//             &(curr_cd->entry_req_.back().req_));
             &(curr_cd->entry_request_req_[tag_to_search].req_));

  dbg << "[FromAny] CHECK TAG : " <<curr_cd->GetCDID().GenMsgTag(tag_to_search)
      << " (Entry Tag: " << tag_to_search << ")" << endl;
  dbg << "Size of data to receive : " << src_data_.len() << " (" << tag_to_search << ")" << endl;
  dbg.flush();
//  curr_cd->entry_req_.push_back(CommInfo()); //getchar();
    ptr_cd()->entry_recv_req_[tag_to_search] = CommInfo();
  // Receive the preserved data to restore the data in application memory
  PMPI_Irecv(src_data_.address_data(), 
             src_data_.len(), 
             MPI_BYTE, 
             MPI_ANY_SOURCE, 
             curr_cd->cd_id_.GenMsgTag(tag_to_search), 
             MPI_COMM_WORLD, 
//             &(curr_cd->entry_req_.back().req_));  
             &(curr_cd->entry_recv_req_[tag_to_search].req_));  
#endif
}



// Bring data handle from parents
CDEntry::CDEntryErrT CDEntry::Restore(void) {
  dbg<<"CDEntry::Restore at level: " << ptr_cd_->level() <<" [";
  bool local_found = false;
  DataHandle *buffer = NULL;
  CDEntry *entry = NULL;
	if( dst_data_.handle_type() == DataHandle::kReference ) {  
    // Restoration from reference
    dbg<< "kReference] Ref name: " << dst_data_.ref_name() << endl; 
    
    int found_level = 0;
    ENTRY_TAG_T tag_to_search = dst_data_.ref_name_tag();
	  entry = ptr_cd()->SearchEntry(tag_to_search, found_level);

    // Remote case
    if(entry != NULL) {  // Local case
      local_found = true;
      // FIXME MPI_Group
      if(entry->dst_data_.node_id() == CDPath::GetCDLevel(found_level)->node_id()) {
        dbg << "\n[CDEntry::Restore] Succeed in finding the entry locally!!" << endl;
        dbg.flush();
        buffer = &(entry->dst_data_);
    
        buffer->set_ref_offset(dst_data_.ref_offset() );   
    
        // length could be different in case we want to describe only part of the preserved entry.
        buffer->set_len(dst_data_.len());     
  
//        dbg<<"addr "<<entry->dst_data_.address_data()<<endl; 
//        dbg<<"addr "<<buffer->address_data()<<endl; 
//  			dbg <<"[Buffer] ref name: "    << buffer->ref_name() 
//                <<", value: "<<*(reinterpret_cast<int*>(buffer->address_data())) << endl;
      }
      else {

#if _MPI_VER
        // Entry was found at this task because the current task is head.
        // So, it should request EntrySend to the task that holds the actual data copy for preservation.
        dbg << "\nIt succeed in finding the entry at this level #"<< found_level <<" of head task!" << endl;
        dbg.flush();
        HeadCD *found_cd = static_cast<HeadCD *>(CDPath::GetCDLevel(found_level)->ptr_cd());
        // It needs some effort to avoid the naming alias problem of entry tags.
      
        // Found the entry!! 
        // Then, send it to the task that has the entry of actual copy
        int target_task_id = entry->dst_data_.node_id().task_in_color();
        
        dbg << "Before SetMailBox(kEntrySend)" << endl;
        dbg.flush(); 
        // SetMailBox for entry send to target
        CDEventT entry_send = kEntrySend;
        found_cd->SetMailBox(entry_send, target_task_id);
    
        dbg << "CHECK TAG : " << found_cd->cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, found_cd->task_in_color())
            << " (Entry Tag: " << tag_to_search << ")" << endl;
        dbg << "CHECK IT OUT : " << target_task_id  << " --> " << found_cd->task_in_color() << " at "<< found_cd->GetCDName() << endl;
        
        dbg << "FOUND " << found_cd->GetCDName() << " " << found_cd->GetNodeID() << ", found level: " << found_level << ", check it out: "<< target_task_id<<endl;
        dbg.flush();
        ENTRY_TAG_T sendBuf[2] = {tag_to_search, static_cast<ENTRY_TAG_T>(myTaskID)};
//        found_cd->entry_search_req_[tag_to_search] = CommInfo();
        found_cd->entry_req_.push_back(CommInfo()); //getchar();
        PMPI_Isend(sendBuf, 
                   2, 
                   MPI_UNSIGNED_LONG_LONG, 
                   target_task_id, 
                   found_cd->cd_id_.GenMsgTagForSameCD(MSG_TAG_ENTRY_TAG, found_cd->task_in_color()),
    //               found_cd->GetCDID().GenMsgTag(tag_to_search), 
//                   MSG_TAG_ENTRY_TAG,
                   found_cd->color(), // Entry sender is also in the same CD task group.
                   &(found_cd->entry_req_.back().req_));
//                   &(found_cd->entry_search_req_[tag_to_search]).req_);

//        CDHandle *curr_cdh = CDPath::GetCurrentCD();
//        while( curr_cdh->node_id().size()==1 ) {
//          if(curr_cdh == CDPath::GetRootCD()) {
//            dbg << "[SetMailBox] there is a single task in the root CD" << endl;
//            assert(0);
//          }
//          // If current CD is Root CD and GetParentCD is called, it returns NULL
//        	curr_cdh = CDPath::GetParentCD(curr_cdh->ptr_cd()->GetCDID().level());
//        } 
//        CDHandle *cdh = CDPath::GetAncestorLevel(found_level);
//        if(!cdh->IsHead()) assert(0);
//        HeadCD *headcd = static_cast<HeadCD *>(cdh->ptr_cd());
//        CDEventT entry_send = kEntrySend;
//        headcd->SetMailBox(entry_send, entry->dst_data_.node_id().task_in_color());
//        dbg << "[CDEntry::Restore] Can't find the entry local. Remote Case!!" << endl;
        cout << "[CDEntry::Restore] Can't find the entry local. Remote Case!! --" << endl;


#endif
      }
    }
    else { // Remote case
      entry = NULL;
      cout << "[CDEntry::Restore] Can't find the entry local. Remote Case!! ==--" << endl;
      dbg << "[CDEntry::Restore] Can't find the entry local. Remote Case!!" << endl;
    } 
	}
  else {  
    // Restoration from memory or file system. Just use the current dst_data_ for it.
    dbg<< "kCopy]" << endl; 
    buffer = &dst_data_;
    dst_data_.address_data();
  }
  cout << "Entry ? " << entry << endl;
  dbg.flush(); 
  return InternalRestore(buffer, local_found);
}

CDEntry::CDEntryErrT CDEntry::InternalRestore(DataHandle *buffer, bool local_found)
{
  dbg<<"CDEntry::InternalRestore ";

  // Handles preservation via reference for remote copy.
  if(buffer == NULL) {

#if _MPI_VER

    dbg<<"Request head to bring the entry" << endl;
    
    // Add this entry for receiving preserved data remotely.
    ENTRY_TAG_T tag_to_search = dst_data_.ref_name_tag();

    if(local_found == false) {
      RequestEntrySearch();
    } 
    else {
      // Receive the preserved data to restore the data in application memory
      dbg << "Found entry locally, but need to bring the entry (" << tag_to_search << ", "
          << ptr_cd()->GetCDID().GenMsgTag(tag_to_search) << ") from remote task in the same CD" << endl;
      dbg << "CHECK TAG : " << tag_to_search << " --> " << ptr_cd()->GetCDID().GenMsgTag(tag_to_search) << endl;
      dbg.flush();
      ptr_cd()->entry_recv_req_[tag_to_search] = CommInfo();
//      ptr_cd()->entry_req_.push_back(CommInfo()); //getchar(); //src_data_.address_data());
      PMPI_Irecv(src_data_.address_data(), 
                 src_data_.len(), 
                 MPI_BYTE, 
                 MPI_ANY_SOURCE, 
                 ptr_cd()->GetCDID().GenMsgTag(tag_to_search), 
                 MPI_COMM_WORLD, 
//                 &(ptr_cd()->entry_req_.back().req_));  
                 &(ptr_cd()->entry_recv_req_[tag_to_search].req_));  
    }



    dbg<<"RequestEntrhSearch Done\n" << endl;
    dbg.flush();

#endif
    return kEntrySearchRemote;
  }
	else if(buffer->handle_type() == DataHandle::kMemory)	{
	  //FIXME we need to distinguish whether this request is on Remote or local for both 
    // when using kOSFile or kMemory and do appropriate operations..

    dbg<<"Local Case -> kMemory"<<endl;
		if(src_data_.address_data() != NULL) {
      dbg << src_data_.len() << " - " << buffer->len() << ", offset : "<<buffer->ref_offset() << endl;
      dbg << src_data_.address_data() << " - " << buffer->address_data() << endl;
      assert( src_data_.len() == buffer->len() );
//      dbg << "sizeof src "<< sizeof(src_data_.address_data()) << ", sizeof dst " <<sizeof(buffer->address_data())<<endl;
			memcpy(src_data_.address_data(), (char *)buffer->address_data()+(buffer->ref_offset()), buffer->len()); 
      dbg<<"memcpy succeeds"<<endl; //dbgBreak();

      dbg<<"memcpy "<< dst_data_.len() <<" size data from "<< dst_data_.address_data() << " to "<< src_data_.address_data() <<endl<<endl;
		}
		else {
			cerr << "Not enough memory." << endl;
		}

	}
	else if( buffer->handle_type() == DataHandle::kOSFile)	{
    dbg<<"CDEntry::Restore -> kOSFile"<<endl;
		//FIXME we need to collect file writes and write as a chunk. We don't want to have too many files per one CD.   

		FILE *fp;
		//  Util::GetUniqueCDFileName(cd_id_, src_data_.address_data(), cd_file); 
    // assume cd_id_ is unique (sequential cds will have different cd_id) and address data is also unique by natural
		const char* cd_file = buffer->file_name_;
		fp = fopen(cd_file, "r");
		if( fp!= NULL )	{
      if( buffer->ref_offset() != 0 ) 
        fseek(fp, buffer->ref_offset(), SEEK_SET); 
			fread(src_data_.address_data(), 1, src_data_.len(), fp);
			fclose(fp);
			return CDEntryErrT::kOK;
		}
		else 
      return CDEntryErrT::kFileOpenError;

	}


	return kOK;
	//Direction is from dst_data_ to src_data_

}



ostream& cd::operator<<(ostream& str, const CDEntry& cd_entry)
{
  return str << "\n== CD Entry Information ================"
             << "\nSource      :\t" << cd_entry.src_data_  
             << "\nDestination :\t" << cd_entry.dst_data_
             << "\nEntry Tag   :\t" << cd_entry.entry_tag_
             << "\nEntry Name  :\t" << tag2str[cd_entry.entry_tag_]
             << "\n========================================\n";
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
      dbg<<"memcpy "<< src_data_.len() <<" size data from "<< src_data_.address_data() << " to "<< dst_data_.address_data() <<endl<<endl;
    }
    else {
      return kOutOfMemory;
      //  ERROR_MESSAGE("Not enough memory.");
    }

  }
  else if( dst_data_.handle_type() == DataHandle::kOSFile ) {
    if( !strcmp(dst_data_.file_name_, INIT_FILE_PATH) ) {
//      char *cd_file = new char[MAX_FILE_PATH];
//      cd::Util::GetUniqueCDFileName(cd_id_, src_data_.address_data(), cd_file); 




      // assume cd_id_ is unique (sequential cds will have different cd_id) and address data is also unique by natural
//      dst_data_.file_name_ = Util::GetUniqueCDFileName(ptr_cd_->GetCDID(), "kOSFILE");
    }

    // FIXME	Jinsuk: we need to collect file writes and write as a chunk. 
    // We don't want to have too many files per one CD. So perhaps one file per chunk is okay.

    FILE *fp;
    //char filepath[1024]; 
    //Util::GetCDFilePath(cd_id_, src_data.address_data(), filepath); 
    // assume cd_id_ is unique (sequential cds will have different cd_id) and address data is also unique by natural
    fp = fopen(dst_data_.file_name_, "w");
    if( fp!= NULL ) {
      fwrite(src_data_.address_data(), sizeof(char), src_data_.len(), fp);
      fclose(fp);
      return kOK;
    }
    else return kFileOpenError;
  }


  return kOK;

}


