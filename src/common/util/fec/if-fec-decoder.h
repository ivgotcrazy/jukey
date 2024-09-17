#pragma once

#include <inttypes.h>
#include <memory>

namespace jukey::util
{

//==============================================================================
// 
//==============================================================================
class IFecDecoder
{
public:
	virtual bool Decode(void* data[], int index[], int size) = 0;
};
typedef std::unique_ptr<IFecDecoder> IFecDecodeUP;

}