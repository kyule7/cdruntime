#include <cstdio>
#include "singleton.h"
//#include "libc_wrapper.h"
using namespace logger;

Singleton::Singleton(void) 
{
  //Initialize();
//  printf("Init\n"); STOPHERE;
}

Singleton::~Singleton(void)
{
//  printf("Singleton destroyed, disabled:%s, replaying %s\n", (logger::disabled)? "True":"False", (logger::replaying)? "True":"False"); //STOPHERE;
}

void Singleton::Initialize(void)
{
  InitMallocPtr();
}

void Singleton::BeginClk(int ftid)
{

}

void Singleton::EndClk(int ftid)
{

}


