#include "luigi-fec-encoder.h"

extern "C"
{
#include "fec.h"
}


namespace jukey::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
LuigiFecEncoder::LuigiFecEncoder(uint32_t k, uint32_t r)
{
	m_encoder = fec_new(k, k + r);
	m_k = k;
	m_r = r;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
LuigiFecEncoder::~LuigiFecEncoder()
{
	if (m_encoder) {
		fec_free(m_encoder);
		m_encoder = nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void LuigiFecEncoder::Encode(void* src[], void* dst[], int size)
{
	for (uint32_t i = m_k; i < m_k + m_r; i++) {
		fec_encode(m_encoder, src, dst[i - m_k], i, size);
	}
}

}