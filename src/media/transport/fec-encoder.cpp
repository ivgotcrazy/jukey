#include "fec-encoder.h"
#include "fec/luigi-fec-encoder.h"
#include "log.h"
#include "protocol.h"

namespace jukey::txp
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
FecEncoder::FecEncoder(IFecEncodeHandler* handler, ISeqAllocator* allocator) 
	: m_handler(handler), m_seq_allocator(allocator)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
FecEncoder::~FecEncoder()
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecEncoder::SetParam(uint8_t k, uint8_t r)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (k > 16 || r > k) {
		LOG_ERR("Set invalid fec param, k:{}, r:{}", k, r);
		return;
	}

	if (k == m_k && r == m_r) {
		LOG_DBG("Set same param, k:{}, r:{}", k, r);
		return;
	}

	m_k = k;
	m_r = r;

	auto segments = m_segments;

	m_segments.clear();
	m_redundants.clear();

	m_group_seq = 0;
	m_data_len = 0;

	if (m_k == 0) {
		m_encoder.reset();
	}
	else {
		m_encoder.reset(new util::LuigiFecEncoder(m_k, m_r));
	}

	LOG_INF("Set fec encoder param success, k:{}, r:{}", m_k, m_r);

	// 修改编码器参数时，残余的数据需要重新编码
	for (const auto& segment : segments) {
		prot::SegHdr* hdr = (prot::SegHdr*)DP(segment);
		LOG_INF("Process segment data, fseq:{}, sseq:{}", hdr->fseq, hdr->sseq);
		ProcessSegmentData(segment);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecEncoder::UpdateDataLen(uint32_t len)
{
	m_segments.clear();
	m_redundants.clear();

	m_data_len = len;

	for (auto i = 0; i < m_r; i++) {
		m_redundants.push_back(com::Buffer(m_data_len, m_data_len));
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecEncoder::ProcessSegmentData(const com::Buffer& buf)
{
	if (m_encoder) {
		if (m_data_len == 0) {
			LOG_INF("First segment, len:{}", buf.data_len);
			UpdateDataLen(buf.data_len);
		}
		else if (m_data_len != buf.data_len) {
			LOG_WRN("Data len changed, orig:{}, new:{}", m_data_len, buf.data_len);
			UpdateDataLen(buf.data_len);
		}

		m_segments.push_back(buf);

		TryFecEncode();
	}
	else {
		uint32_t buf_len = buf.data_len + sizeof(prot::FecHdr);
		com::Buffer fec_buf(buf_len, buf_len);

		prot::FecHdr* hdr = (prot::FecHdr*)DP(fec_buf);
		hdr->ver = 0;
		hdr->K = 0;
		hdr->R = 0;
		hdr->rtx = 0;
		hdr->gseq = 0;
		hdr->group = 0;
		hdr->seq = m_seq_allocator->AllocSeq();

		memcpy(DP(fec_buf) + sizeof(prot::FecHdr), DP(buf), buf.data_len);

		return m_handler->OnFecFrameData(fec_buf);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FecEncoder::WriteSegmentData(const com::Buffer& buf)
{
	assert(buf.data_len != 0);

	std::lock_guard<std::mutex> lock(m_mutex);

	ProcessSegmentData(buf);
}

//------------------------------------------------------------------------------
// FIXME: 需要处理长时间没有更多数据的情况
//------------------------------------------------------------------------------
void FecEncoder::TryFecEncode()
{
	if (m_segments.size() < m_k) return;

	uint8_t** src_data = new uint8_t * [m_k];
	uint8_t** red_data = new uint8_t * [m_r];

	for (auto i = 0; i < m_k; i++) {
		src_data[i] = DP(m_segments[i]);
	}

	for (auto i = 0; i < m_r; i++) {
		red_data[i] = DP(m_redundants[i]);
	}

	m_encoder->Encode((void**)src_data, (void**)red_data, m_data_len);

	uint32_t hdr_len = sizeof(prot::FecHdr);
	uint32_t buf_len = m_data_len + hdr_len;

	for (auto i = 0; i < m_k + m_r; i++) {
		com::Buffer buf(buf_len, buf_len);
		
		prot::FecHdr* hdr = (prot::FecHdr*)DP(buf);
		hdr->ver = 0;
		hdr->K = GET_K(m_k);
		hdr->R = GET_R(m_r);
		hdr->gseq = i;
		hdr->rtx = 0;
		hdr->group = m_group_seq;
		hdr->seq = m_seq_allocator->AllocSeq();
		
		if (i < m_k) {
			memcpy(DP(buf) + hdr_len, src_data[i], m_data_len);
		}
		else {
			memcpy(DP(buf) + hdr_len, red_data[i - m_k], m_data_len);
		}

		m_handler->OnFecFrameData(buf);
	}

	m_group_seq++;
	m_segments.clear();

	delete[] src_data;
	delete[] red_data;
}

}