#include "frame-unpacker.h"
#include "log.h"
#include "protocol.h"

namespace jukey::txp
{


//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AudioFrameUnpacker::AudioFrameUnpacker(IFrameUnpackHandler* handler)
	: m_handler(handler)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
AudioFrameUnpacker::~AudioFrameUnpacker()
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioFrameUnpacker::PushSegment(const com::Buffer& buf)
{
	prot::SegHdr* hdr = (prot::SegHdr*)DP(buf);

	m_last_push_fseq = hdr->fseq;

	com::Buffer& cast_buf = const_cast<com::Buffer&>(buf);
	cast_buf.start_pos += SEG_HDR_LEN;
	cast_buf.data_len -= SEG_HDR_LEN;

	m_handler->OnFrameData(cast_buf);

	m_pushed_frames++;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioFrameUnpacker::TryPushCachedSegments()
{
	for (auto i = m_waiting_frames.begin(); i != m_waiting_frames.end();) {
		if (i->first == m_last_push_fseq + 1) {
			PushSegment(i->second);
			i = m_waiting_frames.erase(i);
			continue;
		}
		break;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void AudioFrameUnpacker::WriteSegmentData(const com::Buffer& buf)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	prot::SegHdr* hdr = (prot::SegHdr*)DP(buf);

	LOG_DBG("segment data, fseq:{}, sseq:{}, last:{}, slen:{}", hdr->fseq,
		hdr->sseq, (uint32_t)hdr->st, hdr->slen);

	// TODO: 定义一个合理的宏
	if (hdr->slen < sizeof(prot::SegHdr) || hdr->slen > 1500) {
		LOG_ERR("Invalid segment len:{}", hdr->slen);
		return;
	}

	if (buf.data_len < SEG_HDR_LEN + sizeof(prot::AudioFrameHdr)) {
		LOG_ERR("Invalid audio data len:{}", buf.data_len);
		return;
	}

	if (m_last_push_fseq == -1) {
		PushSegment(buf);
		return;
	}

	if (hdr->fseq <= m_last_push_fseq) {
		LOG_WRN("Receive pushed audio frame:{}", hdr->fseq);
	}
	else if (hdr->fseq == m_last_push_fseq + 1) {
		PushSegment(buf);
		TryPushCachedSegments();
	}
	else {
		// TODO: 使用宏并选择一个合适的值
		if (m_waiting_frames.size() >= 16) {
			auto iter = m_waiting_frames.begin();
			
			if (iter->first <= m_last_push_fseq + 1) {
				LOG_ERR("Unexpected fseq:{}, last push fseq:{}", iter->first, 
					m_last_push_fseq);
			}
			else {
				m_missed_frames += iter->first - ((uint32_t)m_last_push_fseq + 1);
			}

			LOG_WRN("Erase waiting audio frame:{}", iter->first);
			PushSegment(iter->second);
			m_waiting_frames.erase(iter);
		}

		m_waiting_frames.insert(std::make_pair(hdr->fseq, buf));
		TryPushCachedSegments();
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t AudioFrameUnpacker::GetLossRate()
{
	if (m_missed_frames == 0 && m_pushed_frames == 0) {
		return 0;
	}

	uint32_t loss_rate = (uint32_t)(std::ceil((double)m_missed_frames
		/ (m_missed_frames + m_pushed_frames)));

	return loss_rate;
}

////////////////////////////////////////////////////////////////////////////////
// VideoFrameUnpacker
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
VideoFrameUnpacker::VideoFrameUnpacker(IFrameUnpackHandler* handler)
	: m_handler(handler)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
VideoFrameUnpacker::~VideoFrameUnpacker()
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoFrameUnpacker::CheckFrameMapSize()
{
	if (m_frame_map.size() < KMaxCacheFrameSize) return;

	auto iter = m_frame_map.begin();

	LOG_WRN("Remove waiting frame:{}, segs:{}", iter->first, iter->second.size());

	// 寻找 frame 的最后一个 segment
	uint32_t last_seg_seq = 0;
	auto seg_iter = iter->second.rbegin();
	const prot::SegHdr* hdr = (prot::SegHdr*)DP(seg_iter->second);
	if (hdr->st == 1) {
		last_seg_seq = hdr->sseq;
	}

	// 找到最后一个 segment，则用最后一个 segment 计算丢失的 segment 数量
	if (last_seg_seq != 0) {
		if (last_seg_seq + 1 <= iter->second.size()) {
			LOG_ERR("Error, last seg seq:{}, seg count:{}", last_seg_seq,
				iter->second.size());
		}
		else {
			m_unpack_miss_seg += (last_seg_seq + 1) - iter->second.size();
		}
	}
	// 没有找到最后一个 segment，则使统计平均值来计算
	else {
		if (iter->second.size() < m_segs_per_frame) {
			m_unpack_miss_seg += m_segs_per_frame - iter->second.size();
		}
	}

	// 组帧失败也算是 good segment
	m_unpack_good_seg += iter->second.size();

	// 组帧失败也要更新
	m_last_push_fseq = iter->first;
	
	m_stats_unpacked_frames.insert(iter->first);

	m_frame_map.erase(iter);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoFrameUnpacker::WriteSegmentData(const com::Buffer& buf)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	const prot::SegHdr* hdr = (prot::SegHdr*)DP(buf);

	LOG_DBG("segment data, fseq:{}, sseq:{}, last:{}, slen:{}", hdr->fseq, 
		hdr->sseq, (uint32_t)hdr->st, hdr->slen);

	// TODO: 定义一个合理的宏
	if (hdr->slen < SEG_HDR_LEN || hdr->slen > 1500) {
		LOG_ERR("Invalid segment len:{}", hdr->slen);
		return;
	}

	if (buf.data_len < SEG_HDR_LEN + sizeof(prot::VideoFrameHdr)) {
		LOG_ERR("Invalid video data len:{}", buf.data_len);
		return;
	}

	if (hdr->fseq <= m_last_push_fseq) {
		LOG_DBG("Received outedated segment, fseq:{}, sseq:{}", hdr->fseq,
			hdr->sseq);
		return;
	}

	auto iter = m_frame_map.find(hdr->fseq);
	if (iter != m_frame_map.end()) {
		auto result = iter->second.insert(std::make_pair(hdr->sseq, buf));
		if (result.second) {
			LOG_DBG("Insert segment success, fseq:{}, sseq:{}", hdr->fseq, hdr->sseq);
		}
		else {
			LOG_INF("Insert segment failed, fseq:{}, sseq:{}", hdr->fseq, hdr->sseq);
		}
	}
	else {
		CheckFrameMapSize();

		SegMap seg_map;
		seg_map.insert(std::make_pair(hdr->sseq, buf));
		m_frame_map.insert(std::make_pair(hdr->fseq, seg_map));

		LOG_DBG("Insert segment, fseq:{}, sseq:{}", hdr->fseq, hdr->sseq);
	}

	TryAssembleVideoFrame();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoFrameUnpacker::PushFrame(const com::Buffer& buf, uint32_t fseq, 
	uint32_t segs)
{
	// TODO: 序号环回
	if (fseq != m_last_push_fseq + 1) {
		LOG_WRN("Lost frame, last fseq:{}, curr fseq:{}", m_last_push_fseq, fseq);
	}

	m_last_push_fseq = fseq;
	m_stats_unpacked_frames.insert(fseq);

	m_unpack_good_seg += segs;

	// 计算平均每帧包含多少个 segment
	if (m_segs_per_frame == 0) {
		m_segs_per_frame = segs;
	}
	else {
		m_segs_per_frame = (uint32_t)(std::ceil(
			(double)(m_segs_per_frame * 8 + segs * 2) / 10));
	}

	LOG_DBG("Push frame:{}, segs:{}", fseq, segs);

	m_handler->OnFrameData(buf);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoFrameUnpacker::TryAssembleVideoFrame()
{
	for (auto iter = m_frame_map.begin(); iter != m_frame_map.end();) {
		const SegMap& seg_map = iter->second;
		const prot::SegHdr* hdr = (prot::SegHdr*)DP(seg_map.rbegin()->second);

		// 判断是否已经收到最后一个 segment
		if (hdr->st != 1) {
			return;
		}

		// 判断是否已经收到所有的 segment
		if (seg_map.size() < hdr->sseq + 1) {
			return;
		}

		// 计算帧的长度
		uint32_t frame_len = 0;
		for (const auto& seg_item : seg_map) {
			prot::SegHdr* hdr = (prot::SegHdr*)DP(seg_item.second);
			frame_len += hdr->slen - SEG_HDR_LEN; // 减去头部长度
		}

		LOG_DBG("Assemble frame:{}, segs:{}, len:{}", hdr->fseq, seg_map.size(), 
			frame_len);

		// 组帧
		com::Buffer frame_buf(frame_len, frame_len);
		uint32_t copy_pos = 0;
		for (const auto& [sseq, buf] : seg_map) {
			const prot::SegHdr* h = (prot::SegHdr*)DP(buf);
			uint32_t copy_len = h->slen - SEG_HDR_LEN;
			memcpy(DP(frame_buf) + copy_pos, DP(buf) + SEG_HDR_LEN, copy_len);
			copy_pos += copy_len;
		}

		PushFrame(frame_buf, iter->first, seg_map.size());

		// 组帧完成后删除，继续下一个
		iter = m_frame_map.erase(iter);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t VideoFrameUnpacker::GetLossRate()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// 统计丢帧数量
	uint32_t frame_loss = 0;

	if (!m_stats_unpacked_frames.empty()) {
		uint32_t prev_frame = *m_stats_unpacked_frames.begin();
		for (auto curr_frame : m_stats_unpacked_frames) {
			if (curr_frame > prev_frame + 1) {
				frame_loss += curr_frame - (prev_frame + 1);
			}
			prev_frame = curr_frame;
		}
	}

	// 估算 segment 丢失数量
	m_unpack_miss_seg += frame_loss * m_segs_per_frame;

	uint32_t loss_rate = 0;
	if (m_unpack_miss_seg != 0 || m_unpack_good_seg != 0) {
		loss_rate = (uint32_t)(std::ceil((double)m_unpack_miss_seg
			/ (m_unpack_miss_seg + m_unpack_good_seg)));
	}

	m_unpack_good_seg = 0;
	m_unpack_miss_seg = 0;

	m_stats_unpacked_frames.clear();

	return loss_rate;
}

}