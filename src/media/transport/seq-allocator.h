#pragma once

#include <inttypes.h>
#include <mutex>

namespace jukey::txp
{

class ISeqAllocator
{
public:
	virtual uint32_t AllocSeq() = 0;
};

class SeqAllocator : public ISeqAllocator
{
public:
	// ISeqAllocator
	virtual uint32_t AllocSeq() override;

private:
	uint32_t m_next_seq = 0;
	std::mutex m_mutex;
};

}

