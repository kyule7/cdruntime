#ifndef _PACKER_COMMON_H
#define _PACKER_COMMON_H
#include <stdio.h>

#define DEFAULT_FILEMODE kMPIFile
enum {
  kVolatile       = 0x000, // 0x0X00
  kPosixFile      = 0x100,
  kAIOFile        = 0x200,
  kMPIFile        = 0x400,
  kLibcFile       = 0x800
};

extern void PrintPackerProf(FILE *buf=stdout);
#endif
