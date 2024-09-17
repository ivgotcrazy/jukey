#pragma once

#include "if-fec-encoder.h"

namespace jukey::util
{

//==============================================================================
// 
//==============================================================================
class LuigiFecEncoder : public IFecEncoder
{
public:
	LuigiFecEncoder(uint32_t k, uint32_t r);
	~LuigiFecEncoder();

	// IFecEncoder
	virtual void Encode(void* src[], void* dst[], int size) override;

private:
	void* m_encoder = nullptr;
	uint32_t m_k = 0;
	uint32_t m_r = 0;
};

}