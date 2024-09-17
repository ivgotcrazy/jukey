#include "nack-history.h"
#include "protocol.h"
#include "log.h"
#include "common/util-time.h"
#include "transport-common.h"


namespace jukey::txp
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void NackHistory::SaveFecFrameData(const com::Buffer& buf)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	uint64_t now = util::Now();

	for (auto iter = m_history_data.begin(); iter != m_history_data.end();) {
		if (m_history_data.size() > kNackMaxCacheSize
			|| now > iter->second.create_us + kNackMaxCacheDurationMs * 1000) {
			LOG_DBG("Delete nack history item, seq:{}, size:{}", iter->first,
				m_history_data.size());
			iter = m_history_data.erase(iter);
		}
		else {
			break;
		}
	}

	LOG_FEC_FRAME_DBG("Save fec frame", buf);

	prot::FecHdr* hdr = (prot::FecHdr*)DP(buf);

	// 不缓存 FEC 冗余报文
	if (FEC_K(hdr) == 0 || hdr->gseq < FEC_K(hdr)) {
		hdr->rtx = 1; // 标识为重传报文
		m_history_data.insert(
			std::make_pair(hdr->seq, HistoryEntry(now, buf)));

		LOG_DBG("Insert nack packet, "
			"seq:{}, k:{}, r:{}, rtx:{}, group:{}, gseq:{}, history size:{}",  
			hdr->seq, (uint32_t)hdr->K, (uint32_t)hdr->R, (uint32_t)hdr->rtx, 
			hdr->group, (uint32_t)hdr->gseq, m_history_data.size());
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool NackHistory::FindFecFrameData(uint32_t seq, com::Buffer& buf)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto iter = m_history_data.find(seq);
	if (iter == m_history_data.end()) {
		LOG_WRN("Cannot find nack history data, seq:{}", seq);
		return false;
	}

	buf = iter->second.buf;

	prot::FecHdr* hdr = (prot::FecHdr*)DP(buf);
	LOG_DBG("Find nack item, seq:{}, group:{}, gseq:{}", hdr->seq, hdr->group, 
		(uint32_t)hdr->gseq);

	return true;
}

}