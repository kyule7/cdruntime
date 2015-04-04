#include "cd_interface.h"
#include <cstdio>

int main() {

  CDInterface cd_interface;

  cd_interface.LoadConfiguration("error_injection_desc.conf");

  return 0;
}
