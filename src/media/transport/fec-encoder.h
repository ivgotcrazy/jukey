#pragma once

#include "common-struct.h"
#include "fec/if-fec-encoder.h"
#include "seq-allocator.h"

namespace jukey::txp
{

//==============================================================================
// 
//==============================================================================
class IFecEncodeHandler
{
public:
	virtual void OnFecFrameData(const com::Buffer& buf) = 0;
};

//==============================================================================
// 
//==============================================================================
class FecEncoder
{
public:
	FecEncoder(IFecEncodeHandler* handler, ISeqAllocator* allocator);
	~FecEncoder();

	void WriteSegmentData(const com::Buffer& buf);

	void SetParam(uint8_t k, uint8_t r);

private:
	void TryFecEncode();
	void UpdateDataLen(uint32_t len);
	void ProcessSegmentData(const com::Buffer& buf);

private:
	IFecEncodeHandler* m_handler = nullptr;
	ISeqAllocator* m_seq_allocator = nullptr;
	util::IFecEncoderUP m_encoder;

	uint32_t m_data_len = 0;

	uint16_t m_group_seq = 0;
	uint32_t m_data_seq = 0;

	uint8_t m_k = 0;
	uint8_t m_r = 0;

	std::vector<com::Buffer> m_segments;
	std::vector<com::Buffer> m_redundants;

	std::mutex m_mutex;
};

}