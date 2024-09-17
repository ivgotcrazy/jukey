#include "nack-requester.h"
#include "protocol.h"
#include "log.h"
#include "common/util-time.h"
#include "transport-common.h"


#define MAX_NACK_ITEM_COUNT 1024

using namespace jukey::base;

namespace jukey::txp
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
NackRequester::NackRequester(IComFactory* factory, INackRequestHandler* handler)
	: m_factory(factory), m_handler(handler)
{
	m_timer_mgr = QUERY_TIMER_MGR(m_factory);
	assert(m_timer_mgr);

	com::TimerParam timer_param;
	timer_param.timeout = 10;
	timer_param.timer_type = com::TimerType::TIMER_TYPE_LOOP;
	timer_param.timer_name = "nack requester";
	timer_param.timer_func = [this](int64_t) {
		OnTimer();
	};

	m_timer_id = m_timer_mgr->AllocTimer(timer_param);
	m_timer_mgr->StartTimer(m_timer_id);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
NackRequester::~NackRequester()
{
	if (m_timer_id != INVALID_TIMER_ID) {
		m_timer_mgr->StopTimer(m_timer_id);
		m_timer_mgr->FreeTimer(m_timer_id);
		m_timer_id = INVALID_TIMER_ID;
	}
}

//------------------------------------------------------------------------------
// TODO: 没有处理序列号回环
//------------------------------------------------------------------------------
void NackRequester::OnRecvPacket(const com::Buffer& buf)
{
	const prot::FecHdr* hdr = (prot::FecHdr*)DP(buf);

	LOG_FEC_FRAME_DBG("Received packet", buf);

	if (m_max_recv_seq == -1) {
		m_max_recv_seq = hdr->seq;
		m_max_recv_gseq = hdr->gseq;
		return;
	}

	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_nack_map.size() > MAX_NACK_ITEM_COUNT) {
		uint32_t remove_size = m_nack_map.size() - MAX_NACK_ITEM_COUNT;
		auto iter = m_nack_map.begin();
		std::advance(iter, remove_size);
		m_nack_map.erase(m_nack_map.begin(), iter);
		LOG_WRN("Nack item overflow:{}, remove size:{}", m_nack_map.size(), 
			remove_size);
	}

	// 不连续的序列号，先不用考虑乱序，把丢失报文都当做需要重传的报文
	if (hdr->seq > m_max_recv_seq + 1) {
		for (uint32_t i = (uint32_t)m_max_recv_seq + 1, j = m_max_recv_gseq + 1; 
			i < hdr->seq; i++, j++) {
			// 冗余包不生成 NACK 项
			if (FEC_K(hdr) > 0 && j % (FEC_K(hdr) + FEC_R(hdr)) >= FEC_K(hdr)) {
				continue;
			}

			// 已经收到，但可能在 FEC 解码缓存中
			if (m_pre_recv_pkts.find(i) != m_pre_recv_pkts.end()) {
				LOG_DBG("Found received packet, seq:{}", i);
				continue;
			}

			NackItem item;
			item.create_us = util::Now();
			item.seq = i;
			item.retries = 0;
			item.sent_us = 0;
			m_nack_map.insert(std::make_pair(i, item));

			LOG_INF("Insert nack item, seq:{}, gseq:{}", i, 
				FEC_K(hdr) == 0 ? 0 : j % (FEC_K(hdr) + FEC_R(hdr)));
		}
		m_max_recv_seq = hdr->seq;
		m_max_recv_gseq = hdr->gseq;
	}
	else if (hdr->seq == m_max_recv_seq + 1) {
		m_max_recv_seq = hdr->seq;
		m_max_recv_gseq = hdr->gseq;
	}
	else {
		auto iter = m_nack_map.find(hdr->seq);
		if (iter != m_nack_map.end()) {
			if (hdr->rtx == 0) {
				LOG_INF("Remove nack item by unordered packet, seq:{}, retries:{}", 
					hdr->seq, iter->second.retries);
			}
			else {
				LOG_INF("Remove nack item by retransmit packet, seq:{}, retries:{}", 
					hdr->seq, iter->second.retries);
			}
			m_nack_map.erase(iter);
		}
	}

	// 移除过时元素
	auto it = m_pre_recv_pkts.upper_bound((uint32_t)m_max_recv_seq);
	if (it != m_pre_recv_pkts.end()) {
		// 删除从集合开始到找到的迭代器位置的所有元素
		m_pre_recv_pkts.erase(m_pre_recv_pkts.begin(), it);
		LOG_DBG("Remove pre-received packets, size:{}", m_pre_recv_pkts.size());
	}
	else { // 没有找到大于或等于的元素，则集合已经为空或所有元素都小于
		m_pre_recv_pkts.clear();
		LOG_DBG("Clear pre-received packets");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void NackRequester::UpdateRTT(uint32_t rtt_ms)
{
	m_rtt_ms = rtt_ms;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void NackRequester::OnPreRecvPacket(uint32_t seq)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_pre_recv_pkts.insert(seq);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t NackRequester::GetFirstReqIntervalMs()
{
	return (m_rtt_ms < 20) ? 20 : kNackFirstReqRttMultiple * m_rtt_ms;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void NackRequester::OnTimer()
{
	uint64_t now = util::Now();
	std::vector<uint32_t> nack_list;

	{
		std::lock_guard<std::mutex> lock(m_mutex);

		for (auto iter = m_nack_map.begin(); iter != m_nack_map.end(); ) {
			NackItem& item = iter->second;

			if (item.sent_us == 0) { // 第一次
				if (now > item.create_us + GetFirstReqIntervalMs() * 1000) {
					nack_list.push_back(item.seq);
					item.retries++;
					item.sent_us = now;

					LOG_DBG("Add nack request item, seq:{}, retries:{}", item.seq, 
						item.retries);
				}
			}
			else {
				if (now > item.sent_us + kNackIntervalRttMultiple * m_rtt_ms * 1000) {
					nack_list.push_back(item.seq);
					item.retries++;
					item.sent_us = now;

					LOG_DBG("Add nack request item, seq:{}, retries:{}", item.seq, 
						item.retries);
					
					// 重传限制（次数+时间）
					if (item.retries >= kNackMaxRetries
						|| now >= item.create_us + kNackTimeoutMs * 1000) {
						LOG_INF("Remove nack item:{}, retries:{}, create:{}, now:{}", 
							item.seq, item.retries, item.create_us, now);
						iter = m_nack_map.erase(iter);
						continue;
					}
				}
			}
			++iter;
		}
	}

	if (!nack_list.empty()) {
		uint32_t nack_len = nack_list.size() * sizeof(uint32_t);
		uint32_t buf_len = sizeof(TLV) + nack_len;
		com::Buffer buf(buf_len, buf_len);

		TLV* tlv = (TLV*)DP(buf);
		tlv->type = FeedbackType::FBT_NACK;
		tlv->length = nack_len;

		for (auto i = 0; i < nack_list.size(); i++) {
			uint8_t* p = DP(buf) + sizeof(TLV) + i * sizeof(uint32_t);
			*((uint32_t*)(p)) = nack_list[i];
		}

		m_handler->OnNackRequest(nack_list.size(), buf);

		LOG_DBG("Send nack request, size:{}", nack_list.size());
	}
}

}