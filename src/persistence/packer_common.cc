#include "packer.h"
bool packer::isHead = true;
bool packer::initialized = false;

void packer::Initialize(void)
{
  initialized = true;
}

void packer::Finalize(void)
{
  DeleteFiles();
  initialized = false;
}
