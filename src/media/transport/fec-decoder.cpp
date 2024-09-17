#include "fec-decoder.h"
#include "log.h"
#include "protocol.h"
#include "fec/luigi-fec-decoder.h"
#include "transport-common.h"


#define MAX_FEC_K 16

using namespace jukey::prot;

namespace jukey::txp
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
FecDecoder::FecDecoder(IFecDecodeHandler* handler) : m_handler(handler)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
FecDecoder::~FecDecoder()
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool FecDecoder::UpdateFecParam(uint8_t k, uint8_t r)
{
	if (k > MAX_FEC_K || r > k) {
		LOG_ERR("Invalid fec param, k:{}, r:{}", k, r);
		return false;
	}

	LOG_INF("FEC param changed, {}|{}->{}|{}", m_k, m_r, k, r);

	m_k = k;
	m_r = r;

	if (m_k != 0) {
		m_decoder.reset(new util::LuigiFecDecoder(k, r));
	}
	else {
		m_decoder.reset();
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecDecoder::FlushFecGroup(uint32_t group, const FecGroupMap& frame_map)
{
	for (const auto& item : frame_map) {
		prot::FecHdr* hdr = (prot::FecHdr*)DP(item.second);
		if (hdr->gseq < FEC_K(hdr)) {
			m_handler->OnSegmentData(item.second);
			++m_push_seg_count;
		}
	}

	UpdatePushedGroup(group);

	LOG_WRN("Flush fec group:{}, size:{}", group, frame_map.size());
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecDecoder::RefreshFecGroups()
{
	LOG_INF("Flush all fec groups");

	for (auto& item : m_fec_groups) {
		FlushFecGroup(item.first, item.second);
	}

	m_fec_groups.clear();

	m_stats_pushed_groups.clear();

	m_min_input_seq = -1;
	m_max_input_seq = -1;

	m_last_push_group = -1;

	m_push_seg_count = 0;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecDecoder::InsertFecFrame(const com::Buffer& buf)
{
	const prot::FecHdr* hdr = (prot::FecHdr*)DP(buf);

	// 非冗余乱序报文，直接推出去，TODO：需要处理序列号环回
	if (m_last_push_group != -1 && hdr->group <= m_last_push_group) {
		LOG_INF("Receive pushed fec group frame, last pushed group:{}, seq:{}, "
			"group:{}, gseq:{}, k:{}, r:{}",
			m_last_push_group, hdr->seq, hdr->group, (uint32_t)hdr->gseq,
			(uint32_t)hdr->K, (uint32_t)hdr->R);
		if (hdr->gseq < hdr->K) {
			PushSegment(buf, false);
		}
		return;
	}

	// 查找 group 是否已经存在
	auto iter = m_fec_groups.find(hdr->group);
	if (iter != m_fec_groups.end()) {
		auto result = iter->second.insert(std::make_pair(hdr->gseq, buf));
		if (!result.second) {
			LOG_WRN("Repeat fec frame, group:{}, gseq:{}", hdr->group, 
				(uint32_t)hdr->gseq);
		}
	}
	else {
		if (m_fec_groups.size() >= kMaxFecGroupCacheSize) {
			LOG_INF("Incoming new fec group:{}", hdr->group);
			auto iter = m_fec_groups.begin();
			FlushFecGroup(iter->first, iter->second);
			m_fec_groups.erase(iter);
		}

		FecGroupMap frame_map;
		frame_map.insert(std::make_pair(hdr->gseq, buf));
		m_fec_groups.insert(std::make_pair(hdr->group, frame_map));
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecDecoder::UpdatePushedGroup(uint32_t group)
{
	auto result = m_stats_pushed_groups.insert(group);
	if (!result.second) {
		LOG_WRN("Insert pushed group:{} failed!", group);
	}
	m_push_group_count++;

	LOG_DBG("Insert pushed group:{}", group);

	m_last_push_group = group;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecDecoder::PushGroupWithoutDecode(uint32_t group, const FecGroupMap& frame_map)
{
	LOG_DBG("Push complete fec group:{} without decoding", group);

	for (const auto& item : frame_map) {
		prot::FecHdr* hdr = (prot::FecHdr*)DP(item.second);
		if (hdr->gseq < FEC_K(hdr)) {
			PushSegment(item.second, true);
		}
	}
	UpdatePushedGroup(group);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecDecoder::PushSegment(const com::Buffer& buf, bool stats)
{
	prot::FecHdr* fec_hdr = (prot::FecHdr*)DP(buf);
	prot::SegHdr* seg_hdr = (prot::SegHdr*)(DP(buf) + FEC_HDR_LEN);

	LOG_FEC_FRAME_DBG("Push segment", buf);

	m_handler->OnSegmentData(buf);

	if (stats) m_push_seg_count++;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecDecoder::PushGroupWithDecode(uint32_t group, const FecGroupMap& frame_map)
{
	LOG_DBG("Push complete fec group:{} with decoding", group);

	void** fec_data = new void* [m_k];
	int* data_index = new int[m_k];

	uint32_t i = 0;
	for (auto iter = frame_map.begin(); iter != frame_map.end(); ++iter) {
		if (i >= m_k) break;

		fec_data[i] = DP(iter->second) + FEC_HDR_LEN;
		data_index[i] = iter->first;
		i++;
	}

	// 取第一个 FEC 报文（任意一个都可以）计算解码后第一个报文的 seq
	prot::FecHdr* hdr = (prot::FecHdr*)DP(frame_map.begin()->second);
	uint32_t seq = hdr->seq - hdr->gseq;

	if (m_decoder->Decode(fec_data, data_index, m_data_len - FEC_HDR_LEN)) {
		int i = 0;
		for (auto& item : frame_map) {
			if (i >= m_k) break;

			prot::FecHdr* h = (prot::FecHdr*)DP(item.second);
			h->gseq = i;
			h->seq = seq;

			PushSegment(item.second, true);

			i++;
			seq++;
		}
	}
	else {
		LOG_ERR("Fec decode failed");
	}

	UpdatePushedGroup(group);

	delete[] fec_data;
	delete[] data_index;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecDecoder::DoFecDecode(uint32_t group, const FecGroupMap& frame_map)
{
	assert(!frame_map.empty());

	const prot::FecHdr* hdr = (prot::FecHdr*)DP(frame_map.rbegin()->second);
	if (hdr->gseq >= m_k) { // 有冗余报文，需要解码
		PushGroupWithDecode(group, frame_map);
	}
	else { // 都是原始报文，不需要解码
		PushGroupWithoutDecode(group, frame_map);
	}
}

//------------------------------------------------------------------------------
// 这个策略暂未使用
//------------------------------------------------------------------------------
void FecDecoder::TryFecDecodeOutOfOrder()
{
	auto decode_iter = m_fec_groups.end();

	// 查找可以解码的组
	for (auto iter = m_fec_groups.begin(); iter != m_fec_groups.end(); iter++) {
		if (iter->second.size() >= m_k) {
			decode_iter = iter;
		}
	}

	// 没有可以解码的组
	if (decode_iter == m_fec_groups.end()) {
		return;
	}

	LOG_DBG("Found decodable fec group:{}", decode_iter->first);

	// 可以解码的组之前的不可解码组都推出去，不再等待了
	for (auto iter = m_fec_groups.begin(); iter != decode_iter;) {
		LOG_INF("Push incomplete fec group:{} without decode", iter->first);
		PushGroupWithoutDecode(iter->first, iter->second);
		iter = m_fec_groups.erase(iter);
	}

	// 推出不可解码组后，剩下的第一个项应该就是可解码组
	if (m_fec_groups.empty()) {
		assert(false);
		LOG_ERR("Impossible!!!");
		return;
	}

	auto iter = m_fec_groups.begin();
	if (iter->second.size() < m_k) {
		assert(false);
		LOG_ERR("Impossible!!!");
		return;
	}

	DoFecDecode(iter->first, iter->second);
	iter = m_fec_groups.erase(iter);
}

//------------------------------------------------------------------------------
// 按顺序解码
//------------------------------------------------------------------------------
void FecDecoder::TryFecDecodeInOrder()
{
	// 顺序解码，遇到不能解码的分组退出
	for (auto iter = m_fec_groups.begin(); iter != m_fec_groups.end(); ) {
		// 数据不够无法解码
		if (iter->second.size() < m_k) break;

		// 解码
		DoFecDecode(iter->first, iter->second);

		// 解码后移除
		iter = m_fec_groups.erase(iter);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecDecoder::ProcWithDecode(const com::Buffer& buf)
{
	InsertFecFrame(buf);

	TryFecDecodeInOrder();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecDecoder::ProcWithoutDecode(const com::Buffer& buf)
{
	prot::FecHdr* hdr = (prot::FecHdr*)DP(buf);

	if (FEC_K(hdr) != 0) {
		LOG_ERR("Invalid K:{}", (uint32_t)FEC_K(hdr));
		return;
	}

	if (m_min_input_seq == -1) {
		m_min_input_seq = hdr->seq;
	}

	if (hdr->seq > m_max_input_seq) {
		m_max_input_seq = hdr->seq;
	}

	PushSegment(buf, true);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecDecoder::WriteFecFrame(const com::Buffer& buf)
{
	// TODO: 锁的范围有点大
	std::lock_guard<std::mutex> lock(m_mutex);

	if (buf.data_len <= sizeof(prot::FecHdr)) {
		LOG_ERR("Invalid buffer len:{}", buf.data_len);
		return;
	}

	prot::FecHdr* hdr = (prot::FecHdr*)DP(buf);

	LOG_FEC_FRAME_DBG("Write fec frame", buf);

	// 重传报文直接透传
	if (hdr->rtx == 1) {
		PushSegment(buf, false);
		return;
	}

	// FEC 参数变化
	if (FEC_K(hdr) != m_k || FEC_R(hdr) != m_r) {
		if (!UpdateFecParam(FEC_K(hdr), FEC_R(hdr))) {
			LOG_ERR("Update fec param failed!");
			return;
		}
		RefreshFecGroups();
		m_data_len = buf.data_len;
	}

	// 数据长度变化
	if (buf.data_len != m_data_len) {
		LOG_WRN("Invalid data len:{}, expect len:{}", buf.data_len, m_data_len);
		RefreshFecGroups();
		m_data_len = buf.data_len;
	}

	(FEC_K(hdr) == 0) ? ProcWithoutDecode(buf) : ProcWithDecode(buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t FecDecoder::GetFecLossRate()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	uint32_t orig_seg_count = 0;
	uint32_t loss_rate = 0;

	if (m_k == 0) {
		if (m_min_input_seq != -1 && m_max_input_seq != -1
			&& m_max_input_seq > m_min_input_seq) {
			orig_seg_count = (uint32_t)(m_max_input_seq - m_min_input_seq + 1);
		}
	}
	else {
		if (!m_stats_pushed_groups.empty()) {
			uint32_t loss_group = 0;
			uint32_t prev_group = *m_stats_pushed_groups.begin() - 1;
			for (auto curr_group : m_stats_pushed_groups) {
				loss_group += curr_group - prev_group - 1;
				prev_group = curr_group;
			}
			orig_seg_count = (m_push_group_count + loss_group) * m_k;
		}
	}

	do {
		if (orig_seg_count == 0) {
			LOG_DBG("No origin segment!");
			break;
		}

		if (orig_seg_count < m_push_seg_count) {
			LOG_WRN("Origin segment count:{} less than push segment count:{}",
				orig_seg_count, m_push_seg_count);
			break;
		}

		loss_rate = static_cast<uint32_t>(std::ceil(static_cast<double>(
			orig_seg_count - m_push_seg_count) * 100 / orig_seg_count));

		LOG_DBG("Origin segment count:{}, push segment count:{}, loss rate:{}",
			orig_seg_count, m_push_seg_count, loss_rate);
	} while (false);

	m_push_seg_count = 0;
	m_push_group_count = 0;

	if (m_k == 0) {
		m_min_input_seq = m_max_input_seq = -1;
	}
	else {
		if (!m_stats_pushed_groups.empty()) {
			uint32_t back_group = *m_stats_pushed_groups.rbegin();
			m_stats_pushed_groups.clear();
			m_stats_pushed_groups.insert(back_group);
		}
	}

	return loss_rate;
}

}