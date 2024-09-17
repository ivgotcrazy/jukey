#include "com-factory-impl.h"

using namespace jukey::base;

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
IComFactory* GetComFactory(void)
{
	return ComFactoryImpl::Instance();
}

