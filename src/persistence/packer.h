#ifndef _PACKER_GLOBAL_HEADER_H
#define _PACKER_GLOBAL_HEADER_H

namespace packer {

extern bool isHead;
extern bool initialized;
extern void Initialize(void);
extern void Finalize(void);
extern void DeleteFiles(void);

}
#endif
