#pragma once

#include "if-unknown.h"

namespace test {

class IComTest : public jukey::base::IUnknown
{
public:
	virtual int CalcResult() = 0;
};

}
