#pragma once

#include <map>

#include "common-struct.h"

namespace jukey::txp
{

//==============================================================================
// 
//==============================================================================
class NackHistory
{
public:
	void SaveFecFrameData(const com::Buffer& buf);
	bool FindFecFrameData(uint32_t seq, com::Buffer& buf);

private:
	struct HistoryEntry
	{
		HistoryEntry(uint64_t t, const com::Buffer& b) : create_us(t), buf(b) {}

		uint64_t create_us;
		com::Buffer buf;
	};

	std::map<uint32_t, HistoryEntry> m_history_data;
	std::mutex m_mutex;

	static const uint32_t kNackMaxCacheSize = 40960;
	static const uint32_t kNackMaxCacheDurationMs = 1000;
};

}