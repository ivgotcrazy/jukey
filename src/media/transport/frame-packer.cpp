#include "frame-packer.h"
#include "protocol.h"
#include "log.h"
#include "transport-common.h"

using namespace jukey::com;
using namespace jukey::util;
using namespace jukey::prot;

namespace jukey::txp
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
FramePacker::FramePacker(com::StreamType st, IFramePackHandler* handler)
	: m_handler(handler), m_st(st)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
FramePacker::~FramePacker()
{
	LOG_INF("{}", __FUNCTION__);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void FramePacker::WriteFrameData(const com::Buffer& buf)
{
	uint32_t seg_hdr_len = sizeof(SegHdr);
	uint32_t seg_len = 0; // 不包含头部
	uint8_t mt = 0;
	uint8_t ft = 0;
	uint8_t sl = 0;
	uint8_t tl = 0;
	uint32_t fseq = 0;
	uint32_t ts = 0;

	if (m_st == StreamType::AUDIO) {
		if (buf.data_len > 1024) {
			LOG_ERR("Invalid buffer length:{}", buf.data_len);
			return;
		}

		seg_len = buf.data_len; // 音频数据不需要切分
		mt = (uint8_t)SegPktType::SPT_AUDIO;

		AudioFrameHdr* hdr = (AudioFrameHdr*)DP(buf);
		fseq = hdr->fseq;
		ts = hdr->ts;
	}
	else if (m_st == StreamType::VIDEO) {
		seg_len = 1024; // TODO: 用宏？
		mt = (uint8_t)SegPktType::SPT_VIDEO;

		VideoFrameHdr* hdr = (VideoFrameHdr*)DP(buf);
		ft = hdr->ft;
		sl = hdr->sl;
		tl = hdr->tl;
		ts = hdr->ts;
		fseq = hdr->fseq;
	}
	else {
		LOG_ERR("Invalid stream type:{}", m_st);
		return;
	}

	LOG_DBG("Write frame data, mt:{}, ft:{}, sl:{}, tl:{}, len:{}", mt, ft, sl,
		tl, buf.data_len);

	// 计算切分为多少个 segment
	uint32_t seg_count = (buf.data_len + seg_len - 1) / seg_len;

	uint32_t copy_pos = 0;
	uint32_t seg_seq = 0;

	// segment 为固定长度
	uint32_t total_seg_len = seg_len + seg_hdr_len;

	for (uint32_t i = 0; i < seg_count; i++) {
		// 申请内存
		com::Buffer seg_buf(total_seg_len, total_seg_len);

		// 计算 segment 内容长度，最后一个 segment 需要单独计算
		uint32_t data_len = 
			(i == seg_count - 1) ? (buf.data_len - copy_pos) : seg_len;

		// Build protocol header
		SegHdr* seg_hdr = (SegHdr*)DP(seg_buf);
		seg_hdr->ver = 0;
		seg_hdr->mt = mt;
		seg_hdr->st = (i == seg_count - 1) ? 1 : 0;
		seg_hdr->ft = ft;
		seg_hdr->sl = sl;
		seg_hdr->tl = tl;
		seg_hdr->rsv = 0;
		seg_hdr->slen = data_len + seg_hdr_len; // 包含头部长度
		seg_hdr->sseq = seg_seq++;
		seg_hdr->ts = ts;
		seg_hdr->fseq = fseq;

		// Copy segment data
		memcpy(seg_buf.data.get() + seg_hdr_len, buf.data.get() + copy_pos, 
			data_len);

		// Update copy position
		copy_pos += data_len;

		m_handler->OnSegmentData(seg_buf);

		LOG_DBG("Add frame segment, fseq:{}, sseq:{}, slen:{}", fseq,
			seg_hdr->sseq, (uint32_t)seg_hdr->slen);
	}
}

}