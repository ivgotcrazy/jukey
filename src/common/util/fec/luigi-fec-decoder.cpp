#include "luigi-fec-decoder.h"

extern "C"
{
#include "fec.h"
}


namespace jukey::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
LuigiFecDecoder::LuigiFecDecoder(uint32_t k, uint32_t r)
{
	m_decoder = fec_new(k, k + r);
	m_k = k;
	m_r = r;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
LuigiFecDecoder::~LuigiFecDecoder()
{
	if (m_decoder) {
		fec_free(m_decoder);
		m_decoder = nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool LuigiFecDecoder::Decode(void* data[], int index[], int size)
{
	return (0 == fec_decode(m_decoder, data, index, size));
}

}