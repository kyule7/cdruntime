#include "tuned_cd_handle.h"

namespace tuned {

CDHandle *CD_Init_tuned(int numTask, int myTask, PrvMediumT prv_medium)
{
  cd::CDHandle *root_handle = cd::CD_Init(numTask, myTask, prv_medium);
  CDHandle *tuned_root_handle = new CDHandle(root_handle, 0, "Root");
  CDPath::GetCDPath()->push_back(tuned_root_handle);
  return tuned_root_handle;
}

CDPath *CDPath::uniquePath_ = NULL;

CDHandle *GetCurrentCD(void) 
{ return tuned::CDPath::GetCurrentCD(); }

CDHandle *GetLeafCD(void)
{ return tuned::CDPath::GetLeafCD(); }
  
CDHandle *GetRootCD(void)    
{ return tuned::CDPath::GetRootCD(); }

CDHandle *GetParentCD(void)
{ return tuned::CDPath::GetParentCD(); }

CDHandle *GetParentCD(int current_level)
{ return tuned::CDPath::GetParentCD(current_level); }

}
