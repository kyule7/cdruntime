// under development
//#include "cd_blcr.h"
#include "libcr.h"

int cd_request_restart(char* context_filename)
{
  char cmd[255];
  snprintf(cmd, sizeof(cmd), "cr_restart %s", context_filename);
  return system(cmd);
}

