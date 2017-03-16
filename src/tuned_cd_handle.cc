#include "tuned_cd_handle.h"

namespace tuned {

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
