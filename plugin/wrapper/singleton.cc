#include <singleton.h>
#include <cstdio>
//#include "define.h"
//#include "alloc.h"


//DefineFuncPtr(malloc);
//DefineFuncPtr(calloc);
//DefineFuncPtr(valloc);
//DefineFuncPtr(realloc);
//DefineFuncPtr(memalign);
//DefineFuncPtr(posix_memalign);
//DefineFuncPtr(free);

using namespace log;

Singleton::Singleton(void) 
{
  printf("init?\n");
  Initialize();
}

Singleton::~Singleton(void)
{

}

void Singleton::Initialize(void)
{
  printf(">init?\n");
  InitMallocPtr();
  //disabled = 0;
//  InitFuncPtr(malloc);
//  InitFuncPtr(calloc);
//  InitFuncPtr(valloc);
//  InitFuncPtr(realloc);
//  InitFuncPtr(memalign);
//  InitFuncPtr(posix_memalign);
//  InitFuncPtr(free);
}

void Singleton::BeginClk(enum FTID ftid)
{

}

void Singleton::EndClk(enum FTID ftid)
{

}


char log::ft2str[FTIDNums][64] = { "FTID_reserved"
                            , "malloc"
                            , "calloc"
                            , "valloc"
                            , "realloc"
                            , "memalign"
                            , "posix_memalign"
                            , "free"
                            };

//log::Singleton log::singleton;
//int disabled = 0;
