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

#include "cd.h"
#include "cd_handle.h"
#include "cd_id.h"
//#include "data_handle.h"
#include "cd_entry.h"
#include <stdexcept>
#include <typeinfo>

using namespace cd;
using namespace std;

/// Actual CD Object only exists in a single node and in a single process.
/// Potentially copy of CD Object can exist but it should not be used directly. 
/// We also need to think about when maintaining copy of CD objects, 
/// how are they going to be synchrnoized  (the values, and the entries and all that)

/// Handle would be an accessor to this object. (Kind of an interface for these)
/// If CD Object resides in current process then, it is act as pointer. 
/// If CD Object does not reside in current process, then it will do the appropriate things. 

/// Right now I am making single threaded version so don't consider CDHandle too much 
/// Complication will be delayed until we start developing multithreaded or multi node version.

/// TODO: Desgin decision on CD Tree
/// CD Tree: If CD tree is managed by N nodes, 
/// node (cd_id % N) is where we need to ask for insert, delete, and peek operations. 
/// Distributed over N nodes, and the guide line is cd_id. 
/// If every node that exists handles cd tree, then this means cd tree is always local. 
/// Root CD will, in this case, be located at Node 0 for example. 
/// TODO: how do we implement preempt stop function? 

/// ISSUE Kyushick
/// level+1 when we create children CDs.
/// Originally we do that inside CD object creator, 
/// but it can be inappropriate in some case.
/// ex. cd object can be created by something else I guess..
/// So I think it would be more desirable to increase level
/// inside Create() 

CD::CD()
//  : cd_type_(kStrict), name_("INITIAL_NAME"), sys_detect_bit_vector_(0)
{
  cd_type_ = kStrict; 
  name_ = "INITIAL_NAME"; 
  sys_detect_bit_vector_ = 0;
  // Assuming there is only one CD Object across the entire system we initilize cd_id info here.
  cd_id_ = CDID();

  // Kyushick: Object ID should be unique for the copies of the same object?
  // For now, I decided it as an unique one
  cd_id_.object_id_++;

  Init();  
}

  // FIXME: only acquire root handle when needed. 
  // Most of the time, this might not be required.
  // Kyushick : I cannot fully understand this comment....
CD::CD(CDHandle* cd_parent, 
       const char* name, 
       CDID cd_id, 
       CDType cd_type, 
       uint64_t sys_bit_vector)
//  : cd_type_(cd_type), name_(name), sys_detect_bit_vector_(0), cd_id_(cd_id)
{
  cd_type_ = cd_type; 
  name_ = name; 
  sys_detect_bit_vector_ = 0; 
  cd_id_ = cd_id;
  // FIXME 
  // cd_id_ = 0; 
  // We need to call which returns unique id for this cd. 
  // the id is recommeneded to have pre-allocated sections per node. 
  // This way, we don't need to have race condition to get unique id. 
  // Instead local counter is used to get the unique id.

  // Kyushick: Object ID should be unique for the copies of the same object?
  // For now, I decided it as an unique one
  cd_id_.object_id_++;
//  cout << "cd object is created " << cd_id_.object_id_++ <<endl;
  // FIXME maybe call self generating function here?           
  // cd_self_  = self;  //FIXME maybe call self generating function here?           

  // FIXME 
  // cd_id_.level_ = parent_cd_id.level_ + 1;
  // we need to get parent id ... 
  // but if they are not local, this might be hard to acquire.... 
  // perhaps we should assume that cd_id is always store in the handle ...
  // Kyushick : if we pass cd_parent handle to the children CD object when we create it,
  // this can be resolved. 

  Init();  
}

void CD::Initialize(CDHandle* cd_parent, 
                    const char* name, 
                    CDID cd_id, 
                    CDType cd_type, 
                    uint64_t sys_bit_vector)
{
  cd_type_  = cd_type;
  name_     = name;
  cd_id_    = cd_id;
  cd_id_.object_id_++;
  sys_detect_bit_vector_ = sys_bit_vector;
  Init();  
}

void CD::Init()
{
  ctxt_prv_mode_ = kExcludeStack; 
  cd_exec_mode_  = kSuspension;
  option_save_context_ = 0;
//#if _WORK 
//  path = Path("ssd", "hhd");
//  path.SetSSDPath("./SSDpath/");
//  path.SetHDDPath("./HDDpath/");
//  InitOpenHDD();
//  InitOpenSSD();
//#endif

// Kyushick : I think we already initialize cd_id_ object inside cd object creator (outside of Init method)
// So we do not have to get it here. 
// I think this should be inside CDID object creator because there is no information to pass from CD object to CDID object
//  cd_id_.domain_id_ = Util::GetCurrentDomainID();
//  cd_id_.object_id_ = Util::GenerateCDObjectID();
//  cd_id_.sequential_id_ = 0;
}

/// Kyushick:
/// What if a process is just killed by some reason, and this method could not be invoked?
/// Would it be safe to delete meta data and do proper operation when CD finish?
/// Or how about defining virtual CDErrT DeleteCD() for ~CD() ??
/// And if a process gets some signal for abort, it just call this DeleteCD() and
/// inside this function, we explicitly call ~CD() 
CD::~CD()
{
  // Erase all the CDEntries
//  for(std::list<CDEntry>::iterator it = entry_directory_.begin();
//      it != entry_directory_.end(); ++it) {
//    it->Delete();
//  }

  // Request to add me as a child to my parent
//  cd_parent_->RemoveChild(this);
  //FIXME : This will be done at the CDHandle::Destroy()
}

// 
void CD::GenerateTable(int rankID, int new_color, int new_task)
{
//  int sendBuf[3] = { rankID, new_color, new_task };
//  int recvBuf[3] = { 0, 0, 0 };
//  MPI_Gather(sendBuf, recvBuf, count, mpi_data_type, mpi_op, 
}

void HeadCD::GenerateTable(int rankID, int new_color, int new_task)
{
//  MPI_Gather(sendBuf, recvBuf, count, mpi_data_type, mpi_op, 
}

CD* CD::Create(CDHandle* cd_parent, 
               const char* name, 
               const CDID& cd_id, 
               CDType cd_type, 
               uint64_t sys_bit_vector, 
               CDErrT *cd_err)
{
  CD* new_cd = new CD(cd_parent, name, cd_id, cd_type, sys_bit_vector);
  return new_cd;
}
    

CDErrT CD::Destroy(void)
{
  CDErrT err=CDErrT::kOK;

//GONG
//#if _WORK
//  //When we destroy a CD, we need to delete its log (preservation file)
//  for(std::list<CDEntry>::iterator it = entry_directory_.begin(); 
//      it != entry_directory_.end() ; ++it) {
//
//    bool use_file = false;
//    if(cd_id_.level_<=1)  use_file = true;
//    if( use_file == true)  {
////      if(cd_id_.level_==1) { // HDD
////        it->CloseFile(&HDDlog);  
////      }
////      else { // SSD
////        it->CloseFile(&SSDlog);  
////      }
//    }
//
//  }  // for loop ends
//#endif

  if(GetCDID().level_ != 0) {   // non-root CD


  } 
  else {    // Root CD

  }
  
  delete this;

  return err;
}


/*   CD::Begin()
 *  (1) Call all the user-defined error checking functions. 
 *      Jinsuk: Why should we call error checking function at the beginning?
 *      Kyushick: It doesn't have to. I wrote it long time ago, so explanation here might be quite old.
 *    Each error checking function should call its error handling function.(mostly restore() and reexec())  
 *
 */ 

// Here we don't need to follow the exact CD API this is more like internal thing. 
// CDHandle will follow the standard interface. 
CDErrT CD::Begin(bool collective, const char* label)
{
//  cout<<"inside CD::Begin"<<endl;
  if( cd_exec_mode_ != kReexecution ) { // normal execution
    num_reexecution_ = 0;
    cd_exec_mode_ = kExecution;
  }
  else {
    num_reexecution_++ ;
  }

  return CDErrT::kOK;
}

/*  CD::Complete()
 *  (1) Call all the user-defined error checking functions.
 *      Each error checking function should call its error handling function.(mostly restore() and reexec())  
 *
 */
CDErrT CD::Complete(bool collective, bool update_preservations)
{

//  if( cd_exec_mode_ != kReexecution ) {
//  }
//  else {
//  }

  // Increase sequential ID by one
  cd_id_.sequential_id_++;

  /// It deletes entry directory in the CD (for every Complete() call). 
  /// We might modify this in the profiler to support the overlapped data among sequential CDs.
  DeleteEntryDirectory();

  // TODO ASSERT( cd_exec_mode_  != kSuspension );
  // FIXME don't we have to wait for others to be completed?  
  cd_exec_mode_ = kSuspension; 
  return CDErrT::kOK;
}


/*  CD::preserve(char* data_p, int data_l, enum preserveType prvTy, enum mediumLevel medLvl)
 *  Register data information to preserve if needed.
 *  (For now, since we restore per CD, this registration per cd_entry would be thought unnecessary.
 *  We can already know the data to preserve which is likely to be corrupted in future, 
 *  and its address information as well which is in the current memory space.
 *  Main Purpose: 1. Initialize cd_entry information
 *       2. cd_entry::preserveEntry function call. 
 *          -> performs appropriate operation for preservation per cd_entry such as actual COPY.
 *  We assume that the AS *dst_data could be known from Run-time system 
 *  and it hands to us the AS itself from current rank or another rank.
 *  However, regarding COPY type, it stores the back-up data in current memory space beforehand. 
 *  So it allocates(ManGetNewAllocation) memory space for back-up data
 *  and it copies the data from current or another rank to the address 
 *  for the back-up data in my current memory space. 
 *  And we assume that the data from current or another rank for preservation 
 *  can be also known from Run-time system.
 *  ManGetNewAllocation is for memory allocation for CD and cd_entry.
 *  CD_MALLOC is for memory allocation for Preservation with COPY.
 *
 *  Jinsuk: For re-execution we will use this function call to restore the data. 
 *  So basically it needs to know whether it is in re-execution mode or not.
 *
 */


/*
CDErrT CD::Preserve(void* data, 
                    uint64_t len_in_bytes, 
                    uint32_t preserve_mask, 
                    const char* my_name, 
                    const char* ref_name, 
                    uint64_t ref_offset, 
                    const RegenObject* regen_object, 
                    PreserveUseT data_usage)
{

  // FIXME MALLOC should use different ones than the ones for normal malloc
  // For example, it should bypass malloc wrap functions.
  // FIXME for now let's just use regular malloc call 
  if(cd_exec_mode_ == kExecution ) {
//    return ((CDErrT)kOK==InternalPreserve(data, len_in_bytes, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage)) ? (CDErrT)kOK : (CDErrT)kError;
//    return (kOK==InternalPreserve(data, len_in_bytes, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage)) ? kOK : kError;
    return InternalPreserve(data, len_in_bytes, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage);
  }
  else if(cd_exec_mode_ == kReexecution) {
    // it is in re-execution mode, so instead of preserving data, restore the data.
    // Two options, one is to do the job here, 
    // another is that just skip and do nothing here but do the restoration job in different place, 
    // and go though all the CDEntry and call Restore() method. 
    // The later option seems to be more efficient but it is not clear that 
    // whether this brings some consistency issue as restoration is done at the very beginning 
    // while preservation was done one by one 
    // and sometimes there could be some computation in between the preservations.. (but wait is it true?)
  
    // Jinsuk: Because we want to make sure the order is the same as preservation, we go with  Wait...... It does not make sense... 
    // Jinsuk: For now let's do nothing and just restore the entire directory at once.
    // Jinsuk: Caveat: if user is going to read or write any memory space that will be eventually preserved, 
    // FIRST user need to preserve that region and use them. 
    // Otherwise current way of restoration won't work. 
    // Right now restore happens one by one. 
    // Everytime restore is called one entry is restored. 
    if( iterator_entry_ == entry_directory_.end() ) {
      //ERROR_MESSAGE("Error: Now in re-execution mode but preserve function is called more number of time than original"); 
      // NOT TRUE if we have reached this point that means now we should actually start preserving instead of restoring.. 
      // we reached the last preserve function call. 

      //Since we have reached the last point already now convert current execution mode into kExecution
      //      printf("Now reached end of entry directory, now switching to normal execution mode\n");
      cd_exec_mode_  = kExecution;    
  //  return InternalPreserve(data, len_in_bytes, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage);
      return (kOK==InternalPreserve(data, len_in_bytes, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage)) ? kOK : kError;
    }
    else {
      //     printf("Reexecution mode...\n");
      CDEntry cd_entry = *iterator_entry_;
      iterator_entry_++;
#if _WORK
      bool use_file = (bool)ref_name;
      bool open = true;
//      if(cd_id_.level_<=1)
//        use_file = true;
      if( use_file == true) {

//        if(cd_id_.level_==1) {  // HDD
//          bool _isOpenHDD = isOpenHDD();
//          if(!_isOpenHDD)  
//            OpenHDD(); // set flag 'open_HDD'       
//          return cd_entry.Restore(_isOpenHDD, &HDDlog);
//        }
//        else { // SSD
//          bool _isOpenSSD = isOpenSSD();
//          if(!_isOpenSSD)
//            OpenSSD(); // set flag 'open_SSD'       
//          return cd_entry.Restore(_isOpenSSD, &SSDlog);
//        }
        switch(GetPlaceToPreserve()) {
          case kMemory:
            assert(0);
            break;
          case kHDD:
//            bool _isOpenHDD = isOpenHDD();
            if( !isOpenHDD() )  
              OpenHDD(); // set flag 'open_HDD'       
            return cd_entry.Restore(isOpenHDD(), &HDDlog);
        
          case kSSD:
//            bool _isOpenSSD = isOpenSSD();
            if( !isOpenSSD() )
              OpenSSD(); // set flag 'open_SSD'       
            return cd_entry.Restore(isOpenSSD(), &SSDlog);
        
          case kPFS:
            assert(0);
            break;
          default:
            break;
        }

      }
      else {
        return cd_entry.Restore(open, &HDDlog);
      }
#else
      return cd_entry.Restore() kOK : kError;
#endif
    }

  } // Reexecution ends
  return kError; // we should not encounter this point

}
*/

PrvMediumT CD::GetPlaceToPreserve()
{
#if _MEMORY
  return kMemory;
#else
  return kHDD;
#endif
//  if(GetCDID().level_==1) return kHDD;
//  else return kSSD;
}



CDErrT CD::Preserve(void* data, 
                    uint64_t len_in_bytes, 
                    CDPreserveT preserve_mask, 
                    const char* my_name, 
                    const char* ref_name, 
                    uint64_t ref_offset, 
                    const RegenObject* regen_object, 
                    PreserveUseT data_usage)
{

  // FIXME MALLOC should use different ones than the ones for normal malloc
  // For example, it should bypass malloc wrap functions.
  // FIXME for now let's just use regular malloc call 

  if(cd_exec_mode_  == kExecution ) {      // Normal execution mode -> Preservation
//    cout<<"my_name "<< my_name<<endl;
    switch( InternalPreserve(data, len_in_bytes, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage) ) {
      case CDInternalErrT::kOK            : return CDErrT::kOK;
      case CDInternalErrT::kExecModeError : return CDErrT::kError;
      case CDInternalErrT::kEntryError    : return CDErrT::kError;
      default : assert(0);
    }

  }
  else if(cd_exec_mode_ == kReexecution) { // Re-execution mode -> Restoration
    // it is in re-execution mode, so instead of preserving data, restore the data 
    // Two options, one is to do the job here, 
    // another is that just skip and do nothing here but do the restoration job in different place 
    // and go though all the CDEntry and call Restore() method. The later option seems to be more efficient 
    // but it is not clear that whether this brings some consistency issue as restoration is done at the very beginning 
    // while preservation was done one by one and sometimes there could be some computation in between the preservations.. 
    // (but wait is it true?)
  
    // Jinsuk: Because we want to make sure the order is the same as preservation, we go with  Wait...... It does not make sense... 
    // Jinsuk: For now let's do nothing and just restore the entire directory at once.
    // Jinsuk: Caveat: if user is going to read or write any memory space that will be eventually preserved, 
    // FIRST user need to preserve that region and use them. Otherwise current way of restoration won't work. 
    // Right now restore happens one by one. 
    // Everytime restore is called one entry is restored.
    if( iterator_entry_ != entry_directory_.end() ) { // normal case

//      printf("Reexecution mode start...\n");

      CDEntry cd_entry = *iterator_entry_;
      ++iterator_entry_;

      switch( cd_entry.Restore() ) {
        case CDEntry::CDEntryErrT::kOK            : return CDErrT::kOK; 
        case CDEntry::CDEntryErrT::kOutOfMemory   : return CDErrT::kError;
        case CDEntry::CDEntryErrT::kFileOpenError : return CDErrT::kError;
        default : assert(0);
      }
 
    }
    else {  // abnormal case
      //return CDErrT::kOK;

//      cout<< "Something wrong in Reexecution!!!"<<endl<<endl;  
      // NOT TRUE if we have reached this point that means now we should actually start preserving instead of restoring.. 
      // we reached the last preserve function call. 
      // Since we have reached the last point already now convert current execution mode into kExecution

      ERROR_MESSAGE("Error: Now in re-execution mode but preserve function is called more number of time than original"); 
//      printf("Now reached end of entry directory, now switching to normal execution mode\n");

      cd_exec_mode_  = kExecution;    
      switch( InternalPreserve(data, len_in_bytes, preserve_mask, my_name, ref_name, ref_offset, regen_object, data_usage) ) {
        case CDInternalErrT::kOK            : return CDErrT::kOK;
        case CDInternalErrT::kExecModeError : return CDErrT::kError;
        case CDInternalErrT::kEntryError    : return CDErrT::kError;
        default : assert(0);
      }

    }

//    printf("Reexecution mode finished...\n");
  }   // Re-execution mode ends
  else {  // Suspension mode
    // Is it okay ?
    // Is it possible to call Preserve() at Suspension mode?
    assert(0);
    

  }
  
  return kError; // we should not encounter this point
}


CD::CDInternalErrT CD::InternalPreserve(void *data, 
                                        uint64_t len_in_bytes, 
                                        CDPreserveT preserve_mask, 
                                        const char *my_name, 
                                        const char *ref_name, 
                                        uint64_t ref_offset, 
                                        const RegenObject* regen_object, 
                                        PreserveUseT data_usage)
{

  if(cd_exec_mode_  == kExecution ) { // Normal case
    PRINT_DEBUG("\nNormal execution mode (internal preservation call)\n");

    // Now create entry and add to list structure.
    //FIXME Jinsuk: we don't have the way to determine the storage   
    // Let's move all these allocation deallocation stuff to CDEntry. 
    // Object itself will know better than class CD. 

    CDEntry* cd_entry = 0;
//    if(my_name==0) my_name = "INITIAL_ENTRY";

    // Get cd_entry
    if( preserve_mask == kCopy ) {                // via-copy, so it saves data right now!

      PRINT_DEBUG2("\nPreservation via Copy to %d(memory or file)\n", GetPlaceToPreserve());

      switch(GetPlaceToPreserve()) {
        case kMemory: {
          PRINT_DEBUG("kMemory ----------------------------------\n");
          cd_entry = new CDEntry(DataHandle(DataHandle::kSource, data, len_in_bytes), 
                                 DataHandle(DataHandle::kMemory, 0, len_in_bytes), 
                                 my_name);
          cd_entry->set_my_cd(this);

          CDEntry::CDEntryErrT err = cd_entry->SaveMem();

          entry_directory_.push_back(*cd_entry); 
          // FIXME 
//          if( ref_name != 0 ) entry_directory_map_.emplace(ref_name, *cd_entry);
          if( my_name != 0 ) {
            entry_directory_map_[my_name] = cd_entry;
//            std::cout <<"internal preserve... my_name : "<<my_name
//                      <<", value : "<<*(reinterpret_cast<int*>(cd_entry->dst_data_.address_data())) 
//                      <<", address: " <<cd_entry->dst_data_.address_data()<< std::endl;
          }
          return (err == CDEntry::CDEntryErrT::kOK)? CDInternalErrT::kOK : CDInternalErrT::kEntryError;
        }
        case kHDD: {
          PRINT_DEBUG("kHDD ----------------------------------\n");
          cd_entry = new CDEntry(DataHandle(DataHandle::kSource, data, len_in_bytes), 
                                 DataHandle(DataHandle::kOSFile, 0, len_in_bytes), 
                                 my_name);
          cd_entry->set_my_cd(this); 

          if( !log_handle_.isOpenHDD() ) log_handle_.OpenHDD(); // set flag 'open_HDD'       
          CDEntry::CDEntryErrT err = cd_entry->SaveFile(log_handle_.path.GetHDDPath(), log_handle_.isOpenHDD(), &(log_handle_.HDDlog));

          entry_directory_.push_back(*cd_entry); 
 
//          if( ref_name != 0 ) entry_directory_map_.emplace(ref_name, *cd_entry);
          if( my_name != 0 ) {
            entry_directory_map_[my_name] = cd_entry;
//            std::cout <<"internal preserve... my_name : "<<my_name
//                      <<", value : "<<*(reinterpret_cast<int*>(cd_entry->dst_data_.address_data())) 
//                      <<", address: " <<cd_entry->dst_data_.address_data()<< std::endl;
          }

          return (err == CDEntry::CDEntryErrT::kOK)? CDInternalErrT::kOK : CDInternalErrT::kEntryError;
        }
        case kSSD: {
          PRINT_DEBUG("kSSD ----------------------------------\n");
          cd_entry = new CDEntry(DataHandle(DataHandle::kSource, data, len_in_bytes), 
                                 DataHandle(DataHandle::kOSFile, 0, len_in_bytes), 
                                 my_name);
          cd_entry->set_my_cd(this); 

          if( !log_handle_.isOpenSSD() ) log_handle_.OpenSSD(); // set flag 'open_SSD'       
          CDEntry::CDEntryErrT err = cd_entry->SaveFile(log_handle_.path.GetSSDPath(), log_handle_.isOpenSSD(), &(log_handle_.SSDlog));
          entry_directory_.push_back(*cd_entry);  

//          if( ref_name != 0 ) entry_directory_map_.emplace(ref_name, *cd_entry);
          if( my_name != 0 ) {
            entry_directory_map_[my_name] = cd_entry;
            assert(entry_directory_map_[my_name]);
            assert(entry_directory_map_[my_name]->src_data_.address_data());
//            std::cout <<"internal preserve... my_name : "<<entry_directory_map_[my_name]->name()
//                      <<", value : "<<*(reinterpret_cast<int*>(entry_directory_map_[my_name]->src_data_.address_data())) 
//                      <<", address: " <<entry_directory_map_[my_name]->src_data_.address_data()
//                      <<", address: " <<cd_entry->src_data_.address_data()<< std::endl;

          }
          return (err == CDEntry::CDEntryErrT::kOK)? CDInternalErrT::kOK : CDInternalErrT::kEntryError;
        }
        case kPFS: {
          PRINT_DEBUG("kPFS ----------------------------------\n");
          cd_entry = new CDEntry(DataHandle(DataHandle::kSource, data, len_in_bytes), 
                                 DataHandle(DataHandle::kOSFile, 0, len_in_bytes), 
                                 my_name);
          cd_entry->set_my_cd(this); 
          entry_directory_.push_back(*cd_entry);  
//          if( ref_name != 0 ) entry_directory_map_.emplace(ref_name, *cd_entry);
          if( my_name != 0 ) entry_directory_map_[my_name] = cd_entry;
          PRINT_DEBUG("\nPreservation to PFS which is not supported yet. ERROR\n");
          assert(0);
          break;
        }
        default:
          break;
      }

    }
    else if( preserve_mask == kRef ) { // via-reference

      PRINT_DEBUG2("\nPreservation via %d (reference)\n", GetPlaceToPreserve());

      // set handle type and ref_name/ref_offset
      cd_entry = new CDEntry(DataHandle(DataHandle::kSource, data, len_in_bytes), 
                             DataHandle(DataHandle::kReference, 0, len_in_bytes, ref_name, ref_offset), 
                             my_name);
      cd_entry->set_my_cd(this); // this required for tracking parent later.. this is needed only when via ref
      entry_directory_.push_back(*cd_entry);  
//      entry_directory_map_.emplace(ref_name, *cd_entry);
//      entry_directory_map_[ref_name] = *cd_entry;

      return CDInternalErrT::kOK;
      
    }
    else if( preserve_mask == kRegen ) { // via-reference

      //TODO
      assert(0);
      return CDInternalErrT::kOK;
      
    }

    PRINT_DEBUG("\nUnsupported preservation type\n");
    assert(0);
    return CDInternalErrT::kEntryError;

  }
  else { // Abnormal case
    PRINT_DEBUG("\nReexecution mode (internal preservation call) which is wrong, it outputs kExecModeError\n");
    assert(0);
  }

  // In the normal case, it should not reach this point. It is an error.
  PRINT_DEBUG("\nkExecModeError\n");
  assert(0);
  return kExecModeError; 
}

/* RELEASE
#ifdef IMPL

// Only for reference, this section is not activated
int CD::Preserve(char* data_p, int data_l, enum preserveType prvTy, enum mediumLevel medLvl)
{

  abstraction_trans_func(); // FIXME

  AS* src_data = new AS(cd_self->rankID, SOURCE, data_p, data_l); //It is better to define AS src/dst_data here because we can know rankID.
  switch(prvTy){
    case COPY:
      // AS *DATA_dest = (AS *) manGetNewAllocation(DATA, mediumType);  
      // DATA is passed to figure out the length we need
      if(medLvl==DRAM) {
        char* DATA = DATA_MALLOC(data_l);
        AS* dst_data = new AS(cd_self->rankID, DESTINATION, DATA, data_l);
        if(!DATA){  // FIXME 
          fprintf(stderr, "ERROR DATA_MALLOC FAILED due to insufficient memory");
          return FAILURE;
        }
      } else {
        char* fp = GenerateNewFile(WorkingDir);
        AS* dst_data = new AS(cd_self->rankID, DESTINATION, medLvl, fp);
      }
      cd_entry *entry  = new cd_entry(RankIDfromRuntime, src_data, dst_data, enum preserveType prvTy, enum mediumLevel medLvl, NULL);
      entry->preserveEntry(); // we need to add entry to some sort of list. 
      break;

      ///////////////////////////////////////////////////////////////////////////////////////////
      //song
    case REFER:
      //find parent's (grandparent's or even earlier ancestor's) preserved entry
      //need some name matching
      //TODO: if parent also uses preserve via refer, is the entry in this child cd pointing to its parent's entry, or grandparent's entry??
      //Seonglyong: I think if parent's preserved entry is "via REFER", to let the corresponding entry point (refer to) the original entry 
      //whether it is grandparent's or parent of grandparent's entry will be reasonable by copying the parent's entry. More efficient than searching through a few hops.
      //TODO: need to determine to when to find the entry? in preservation or restoration?
      //    1) in preservation, find the refer entry in parent (or earlier ancestors), set the entry of this cd to be a pointer to that ancestor 
      //    (or copy all the entry information to this entry --> this is messier, and should not be called preserve via refer...)
      //    2) or we could do all those stuff in the restoration, when preserve, we only mark this entry to be preserve via reference, 
      //    and when some errors happen, we could follow the pointer to parents to find the data needed. 
      //Seonglyong: "In restoration" would be right. In terms of efficiency, we need to search the "preserved" entry via the tag in the restoration. 
      cd_handle* temp_parent = cd_parent;
      std::list<cd_entry*>::iterator it_entry = (temp_parent->cdInstance)->entryDirectory.begin();
      for (;it_entry!=(temp_parent->cdInstance)->entryDirectory.end();it_entry++){
        if (it_entry->entryNameMatching(EntryName)) { //TODO: need a parameter of EntryName
          //    not name, but tags...
          //
        }       
      }

      //TODO: should the entry of this cd be just a pointer to parent? 
      //or it will copy all the entry information from parent?
      //SL: Pointer to parent or one of ancestors. What is the entry information? maybe not the preserved data (which is not REFER type anymore) 
      //Whenever more than one hop is expected, we need to copy the REFER entry (which is kind of pointer to ancestor's entry or application data) 
      cd_entry *entry  = new cd_entry(RankIDfromRuntime, src_data, dst_data, prvTy, medLvl, NULL);
      entry->preserveEntry();
      break;
      ///////////////////////////////////////////////////////////////////////////////////////////

    case REGEN:
      customRegenFunc = getRegenFunc(); //Send request to Run-time to get an appropriate regenFunc
      cd_entry *entry  = new cd_entry(RankIDfromRuntime, src_data, dst_data, prvTy, medLvl, customRegenFunc);
      entry->preserveEntry();
      break;
    default:
      fprintf(stderr, "ERROR CD::Preserve: Unknown restoration type: %d\n", prvTy);
      return FAILURE;
  }

  return CDErrT::kOK;

}

#endif
*/




/*  CD::Restore()
 *  (1) Copy preserved data to application data. It calls restoreEntry() at each node of entryDirectory list.
 *
 *  (2) Do something for process state
 *
 *  (3) Logged data for recovery would be just replayed when reexecuted. We need to do something for this.
 */

//Jinsuk: Side question: how do we know if the recovery was successful or not? 
//It seems like this is very important topic, we could think the recovery was successful, 
//but we still can miss, or restore with wrong data and the re-execute, 
//for such cases, do we have a good detection mechanisms?
// Jinsuk: 02092014 are we going to leave this function? 
//It seems like this may not be required. Or for optimization, we can do this. 
//Bulk restoration might be more efficient than fine grained restoration for each block.
CDErrT CD::Restore()
{


  // this code section is for restoring all the cd entries at once. 
  // Now this is defunct. 

  /*  for( std::list<CDEntry*>::iterator it = entry_directory_.begin(), it_end = entry_directory_.end(); it != it_end ; ++it)
      {
      (*it)->Restore();
      } */

  // what we need to do here is just reset the iterator to the beginning of the list.
//  cout<<"CD::Restore()"<<endl;
  iterator_entry_ = entry_directory_.begin();
//  cout<<"entry dir size : "<<entry_directory_.size()<<", entry dir map size : "<<entry_directory_map_.size()<<endl;
//  cout<<"dst addr : "<<iterator_entry_->dst_data_.address_data()<<", size: "<<iterator_entry_->dst_data_.len()<< endl<<endl;

//  assert(iterator_entry_->dst_data_.address_data() != 0);

  //TODO currently we only have one iterator. This should be maintined to figure out the order. 
  // In case we need to find reference name quickly we will maintain seperate structure such as binary search tree and each item will have CDEntry *.

  //Ready for long jump? Where should we do that. here ? for now?

  return CDErrT::kOK;
}



/*  CD::detect()
 *  (1) Call all the user-defined error checking functions.
 *    Each error checking function should call its error handling function.(mostly restore() and reexec())  
 *  (2) 
 *
 *  (3)
 *
 */
CDErrT CD::Detect()
{

  return CDErrT::kOK;
}


CDErrT CD::Assert(bool test)
{

  if(test == false) {
    //Restore the data  (for now just do only this no other options for recovery)
    Restore();
    Reexecute();
  }

  return CDErrT::kOK;
}

CDErrT CD::Reexecute(void)
{
  cd_exec_mode_ = kReexecution; 

  //TODO We need to make sure that all children has stopped before re-executing this CD.
  Stop();

//  if(IsHead()){
//  }
//  else {
//  }

  //TODO We need to consider collective re-start. 
  if(ctxt_prv_mode_ == kExcludeStack) {
    printf("\nlongjmp\n\n");
    longjmp(jump_buffer_, 1);
  }
  else if (ctxt_prv_mode_ == kIncludeStack) {
    printf("\nsetcontext\n\n");
    setcontext(&ctxt_); 
  }

  return CDErrT::kOK;
}

// Let's say re-execution is being performed and thus all the children should be stopped, 
// need to provide a way to stop currently running task and then re-execute from some point. 
CDErrT CD::Stop(CDHandle* cdh)
{
  //TODO Stop current CD.... here how? what needs to be done? 
  // may be wait for others to complete? wait for an event?
  //if current thread (which stop function is running) and the real cd's thread id is different, 
  //that means we need to stop real cd's thread..
  //otherwise what should we do here? nothing?

  // Maybe blocking Recv here? or MPI_Wait()?
  
  return CDErrT::kOK;
}

CDErrT CD::Resume(void)
{
  return CDErrT::kOK;
}

CDErrT CD::AddChild(CDHandle* cd_child) 
{ 
//  std::cout<<"sth wrong"<<std::endl; 
//  getchar(); 
  // Do nothing?
  return CDErrT::kOK;  
}

CDErrT CD::RemoveChild(CDHandle* cd_child) 
{
//  std::cout<<"sth wrong"<<std::endl; 
//  getchar(); 
  // Do nothing?
  return CDErrT::kOK;
}

HeadCD::HeadCD()
{}

HeadCD::HeadCD( CDHandle* cd_parent, 
                    const char* name, 
                    CDID cd_id, 
                    CDType cd_type, 
                    uint64_t sys_bit_vector)
  : CD(cd_parent, name, cd_id, cd_type, sys_bit_vector)
{}

HeadCD::~HeadCD()
{}

CDErrT HeadCD::Stop(CDHandle* cdh)
{

//  if(IsLocalObject()){
//    if( !cd_children_.empty() ) {
//      // Stop children CDs
//      int res;
//      std::list<CD>::iterator itstr = cd_children_.begin(); 
//      std::list<CD>::iterator itend = cd_children_.end();
//      while(1) {
//        if(itstr < itend) {
//          res += (*it).Stop(cdh);
//        }
//        if(res == cd_children_.size()) {
//          break;
//        }
//      }
//    
//    }
//    else {  // terminate
//      return 1;
//    }
//  }
//  else {
//    // return RemoteStop(cdh);
//  }

  return CDErrT::kOK;
}


CDErrT HeadCD::Resume(void)
{
  //FIXME: return value needs to be fixed 

//  for( std::list<CD>::iterator it = cd_children_.begin(), it_end = cd_children_.end(); 
//       it != it_end ; ++it) {
//    (*it).Resume();
//  }

  return CDErrT::kOK;
}


CDErrT HeadCD::AddChild(CDHandle* cd_child) 
{
  cd_children_.push_back(cd_child);

  return CDErrT::kOK;  
}


CDErrT HeadCD::RemoveChild(CDHandle* cd_child) 
{
  //FIXME Not optimized operation. 
  // Search might be slow, perhaps change this to different data structure. 
  // But then insert operation will be slower.
  cd_children_.remove(cd_child);

  return CDErrT::kOK;
}


CDHandle* HeadCD::cd_parent(void)
{
  return cd_parent_;
}

void HeadCD::set_cd_parent(CDHandle* cd_parent)
{
  cd_parent_ = cd_parent;
}




CDEntry* CD::InternalGetEntry(std::string entry_name) 
{
  try {
    auto it = entry_directory_map_.find(entry_name.c_str());
    if(it == entry_directory_map_.end()) {
//      std::cout<<"[InternalGetEntry] There is no entry for reference of "<< entry_name.c_str() 
//               <<" at CD level " << GetCDID().level_ << std::endl;
      //getchar();
      return NULL;
    }
    else {
      CDEntry* cd_entry = entry_directory_map_.find(entry_name.c_str())->second;
      
//      std::cout<<"[InternalGetEntry] ref_name: " <<entry_directory_map_[entry_name.c_str()]->dst_data_.address_data()
//               << ", address: " <<entry_directory_map_[entry_name.c_str()]->dst_data_.address_data()<< std::endl;
      return cd_entry;
    }
  }
  catch (const std::out_of_range &oor) {
    //std::cerr << "Out of Range error: " << oor.what() << '\n';
    return 0;
  }
}

void CD::DeleteEntryDirectory(void)
{
//  cout<<"Delete Entry In"<<endl; getchar();
  for(std::list<CDEntry>::iterator it = entry_directory_.begin();
      it != entry_directory_.end(); ++it) {
    it->Delete();
  }

//  for(std::map<std::string, CDEntry*>::iterator it = entry_directory_map_.begin();
//      it != entry_directory_map_.end(); ++it) {
//    //entry_directory_map_.erase(it);
//  }
//  cout<<"Delete Entry Out"<<endl; getchar();
}



/*  CD::setDetectFunc(void *() custom_detect_func)
 *  (1) 
 *      
 *  (2) 
 *
 *  (3)
 *
 */

//int CD::AddDetectFunc(void *() custom_detect_func)
//{
//
//  return CDErrT::kOK;
//}

// Old version of Creat()
//CDHandle CD::Create(enum CDType cd_type, CDHandle &parent)
//CD* CD::Create(CDHandle* parent, const char* name, CDID new_cd_id, CDType cd_type, uint64_t sys_bit_vector)
//{
//  CD* new_cd = new CD(parent, name, new_cd_id, cd_type, sys_bit_vector);
//  return new_cd;

//  CD* cd = new CD(cd_type, parent);
  // level should be +1 to the parent's level id

//  CDHandle self_handle;


  // CD ID could be look like domain.level.object_unique_id.sequential_id 
  // sequential_id is zero when the object have not yet begun. 
  // after that for each begin() we increase sequential_id by one. 
  // For collective begin, even though it gets called multiple times, just increase by one of course.

//  self_handle.Initialize(cd,cd_id(), Util::GetCurrentTaskID(), Util::GetCurrentProcessID() ); 


  //FIXME TODO Register this to CD Tree  


  //  CDHandle self; // having self at CD instance does not make any sense. This will completely deleted.
  //  self.set_cd(cd);

  //self.set_cd_id(cd_id); // cd_id is unknown at this time. let's do this when CD::Begin() is called
  //FIXME need to put rank id and process id  
//  return self_handle;    
//}
