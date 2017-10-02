#include <cstdio>
#include "singleton.h"
#include "libc_wrapper.h"
using namespace logger;

Singleton::Singleton(void) 
{
  Initialize();
//  printf("Init\n"); getchar();
}

Singleton::~Singleton(void)
{
  printf("Singleton destroyed, disabled:%s, replaying %s\n", (logger::disabled)? "True":"False", (logger::replaying)? "True":"False"); getchar();
}

void Singleton::Initialize(void)
{
  InitMallocPtr();
}

void Singleton::BeginClk(enum FTID ftid)
{

}

void Singleton::EndClk(enum FTID ftid)
{

}


