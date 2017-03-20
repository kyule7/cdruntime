#include "tuned_cd_handle.h"
#include "sys_err_t.h"
#include "cd_def_preserve.h"
#include "cd_global.h"

#include <assert.h>
namespace tuned {

CDHandle *CD_Init(int numTask, int myTask, PrvMediumT prv_medium)
{
  tuning_enabled = true;
#if CD_TUNING_ENABLED
  char *cd_config_file = getenv("CD_CONFIG_FILENAME");
  if(cd_config_file != NULL) {
    cd::config.LoadConfig(cd_config_file);
  } else {
    cd::config.LoadConfig(CD_DEFAULT_CONFIG);
  }
#endif
  cd::CDHandle *root_handle = cd::CD_Init(numTask, myTask, prv_medium);
  CDHandle *tuned_root_handle = new CDHandle(root_handle, 0, DEFAULT_ROOT_LABEL);

  //tuned_root_handle->UpdateParam(0, 0);
  CDPath::GetCDPath()->push_back(tuned_root_handle);
  return tuned_root_handle;
}

void CD_Finalize(void)
{
  cd::CD_Finalize();
  assert(CDPath::GetCDPath()->size() == 1);
  delete CDPath::GetCDPath()->back(); // delete root
  CDPath::GetCDPath()->pop_back();
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

void tuned::AddHandle(CDHandle *handle)
{
  CDPath::GetCDPath()->push_back(handle);
}

void tuned::DeleteHandle() 
{
  delete CDPath::GetCDPath()->back();
  CDPath::GetCDPath()->pop_back();
}
