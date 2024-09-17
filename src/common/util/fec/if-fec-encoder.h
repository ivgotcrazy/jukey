#pragma once

#include <inttypes.h>
#include <memory>

namespace jukey::util
{

//==============================================================================
// 
//==============================================================================
class IFecEncoder
{
public:
	virtual void Encode(void* src[], void* dst[], int size) = 0;
};
typedef std::unique_ptr<IFecEncoder> IFecEncoderUP;

}