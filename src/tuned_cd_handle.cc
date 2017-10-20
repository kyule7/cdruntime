#include "tuned_cd_handle.h"
#include "sys_err_t.h"
#include "cd_def_preserve.h"
#include "cd_global.h"
#include "packer.h"
//#include "util.h"

#include <assert.h>
namespace tuned {

CDHandle *CD_Init(int numTask, int myTask, PrvMediumT prv_medium)
{
  TunedPrologue();
  tuning_enabled = true;
#if CD_RUNTIME_ENABLED
  char *cd_config_file = getenv("CD_CONFIG_FILENAME");
  if(cd_config_file != NULL) {
    cd::config.LoadConfig(cd_config_file, myTask);
  } else {
    cd::config.LoadConfig(CD_DEFAULT_CONFIG, myTask);
  }
  cd::CDHandle *root_handle = cd::CD_Init(numTask, myTask, prv_medium);
#else 
//  cd::myTaskID = myTask;
//  cd::totalTaskSize = numTask;
  cd::InitDir(myTask, numTask);
  packer::SetHead(myTask == 0);
  cd::CDHandle *root_handle = NULL;
#endif
  CDHandle *tuned_root_handle = new CDHandle(root_handle, 0, DEFAULT_ROOT_LABEL);
  CDPath::GetCDPath()->push_back(tuned_root_handle);
  TunedEpilogue();
  printf("tuned root:%p %d\n", root_handle, logger::disabled);
  return tuned_root_handle;
}

void CD_Finalize(void)
{
  TunedPrologue();
#if CD_RUNTIME_ENABLED
  cd::CD_Finalize();
#endif
  CD_ASSERT_STR(CDPath::GetCDPath()->size() == 1, 
      "path size:%lu\n", CDPath::GetCDPath()->size());
  delete CDPath::GetCDPath()->back(); // delete root
  CDPath::GetCDPath()->pop_back();
  TunedEpilogue();
#if CD_LIBC_LOGGING
  logger::Fini();
#endif


  logger::disabled = true;
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


void AddHandle(CDHandle *handle)
{
  CDPath::GetCDPath()->push_back(handle);
}

void DeleteHandle(void) 
{
  delete CDPath::GetCDPath()->back();
  CDPath::GetCDPath()->pop_back();
}


} // namespace tuned ends
