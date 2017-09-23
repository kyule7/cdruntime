#include <cstdio>
#include "singleton.h"

using namespace log;

Singleton::Singleton(void) 
{
  Initialize();
}

Singleton::~Singleton(void)
{

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


