#include <iostream>

#include "com-test.h"

namespace test
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ComTest::ComTest(IComFactory* factory) : ProxyUnknown(nullptr)
{
	std::cout << "ComTest::ComTest" << std::endl;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ComTest::~ComTest()
{
	std::cout << "ComTest::~ComTest" << std::endl;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
IUnknown* ComTest::CreateInstance(IComFactory* factory, const char* name,
	const char* owner)
{
	return new ComTest(factory);
}

void* ComTest::NDQueryInterface(const char* riid)
{
	if (strcmp("ComTest_IID", riid) == 0) {
		return (IComTest*)this;
	}
	else {
		return nullptr;
	}
}

int ComTest::CalcResult()
{
	return 100;
}

}
