#pragma once

#include "if-fec-decoder.h"

namespace jukey::util
{

//==============================================================================
// 
//==============================================================================
class LuigiFecDecoder : public IFecDecoder
{
public:
	LuigiFecDecoder(uint32_t k, uint32_t r);
	~LuigiFecDecoder();

	// IFecDecoder
	virtual bool Decode(void* data[], int index[], int size) override;
	
private:
	void* m_decoder = nullptr;
	uint32_t m_k = 0;
	uint32_t m_r = 0;
};

}