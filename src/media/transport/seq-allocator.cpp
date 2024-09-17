#include "seq-allocator.h"

namespace jukey::txp
{

uint32_t SeqAllocator::AllocSeq()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_next_seq == 0xFFFFFFFF) {
		m_next_seq = 0;
		return 0xFFFFFFFF;
	}
	else {
		return m_next_seq++;
	}
}

}