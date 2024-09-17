#pragma once

#include "if-com-test.h"
#include "proxy-unknown.h"

using namespace jukey::base;

namespace test {

class ComTest : public ProxyUnknown, public IComTest
{
public:
	ComTest(IComFactory* factory);
	~ComTest();

	COMPONENT_IUNKNOWN_IMPL
	COMPONENT_FUNCTION_DECL

	virtual int CalcResult() override;
};

}
