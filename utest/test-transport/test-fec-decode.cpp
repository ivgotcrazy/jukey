#include "gtest/gtest.h"
#include "fec-decoder.h"
#include "fec-encoder.h"
#include "frame-packer.h"
#include "frame-unpacker.h"
#include "protocol.h"
#include "seq-allocator.h"

using namespace jukey::com;
using namespace jukey::txp;
using namespace jukey::prot;

class PackHandler : public IFramePackHandler
{
public:
	void OnSegmentData(const Buffer& buf) override
	{
		m_segments.push_back(buf);
	}

	std::vector<Buffer> m_segments;
};

class UnpackHandler : public IFrameUnpackHandler
{
public:
	virtual void OnFrameData(const Buffer& buf) override
	{
		m_frames.push_back(buf);
	}

	std::vector<Buffer> m_frames;
};

class FecEncodeHandler : public IFecEncodeHandler
{
public:
	virtual void OnFecFrameData(const Buffer& buf) override
	{
		m_fec_frames.push_back(buf);
	}

	std::vector<Buffer> m_fec_frames;
};

class FecDecodeHandler : public IFecDecodeHandler
{
public:
	virtual void OnSegmentData(const Buffer& buf) override
	{
		m_segments.push_back(buf);
	}

	std::vector<Buffer> m_segments;
};

TEST(FecDecode, Normal)
{
	//
	// FEC encode
	//
	
	SeqAllocator allocator;
	FecEncodeHandler encode_handler;
	FecEncoder encoder(&encode_handler, &allocator);

	encoder.SetParam(2, 1);

	Buffer seg1(512, 512);
	Buffer seg2(512, 512);

	encoder.WriteSegmentData(seg1);
	encoder.WriteSegmentData(seg2);

	EXPECT_EQ(encode_handler.m_fec_frames.size(), 3);

	//
	// FEC decode
	//

	FecDecodeHandler decode_handler;
	FecDecoder decoder(&decode_handler);

	// 1) all original data
	decoder.WriteFecFrame(encode_handler.m_fec_frames[0]);
	decoder.WriteFecFrame(encode_handler.m_fec_frames[1]);

	EXPECT_EQ(decode_handler.m_segments.size(), 2);

	decode_handler.m_segments.clear();

	// 2) lost the first original data
	decoder.WriteFecFrame(encode_handler.m_fec_frames[1]);
	decoder.WriteFecFrame(encode_handler.m_fec_frames[2]);

	EXPECT_EQ(decode_handler.m_segments.size(), 2);

	decode_handler.m_segments.clear();

	// 3) lost the second original data
	decoder.WriteFecFrame(encode_handler.m_fec_frames[0]);
	decoder.WriteFecFrame(encode_handler.m_fec_frames[2]);

	EXPECT_EQ(decode_handler.m_segments.size(), 2);
}

TEST(FecDecode, video_full)
{
	//
	// Pack
	//

	PackHandler pack_handler;
	FramePacker packer(StreamType::VIDEO, &pack_handler);

	Buffer buf(8000, 8000);

	VideoFrameHdr* hdr = (VideoFrameHdr*)DP(buf);
	hdr->ver = 1;
	hdr->codec = 1;
	hdr->ext = 0;
	hdr->w = 1;
	hdr->h = 1;
	hdr->fseq = 100;
	hdr->sl = 1;
	hdr->tl = 2;
	hdr->ts = 100;
	hdr->ft = 1;

	packer.WriteFrameData(buf);

	EXPECT_EQ(pack_handler.m_segments.size(), 8);

	//
	// FEC encode
	//

	SeqAllocator allocator;
	FecEncodeHandler encode_handler;
	FecEncoder encoder(&encode_handler, &allocator);

	encoder.SetParam(8, 4);

	encoder.WriteSegmentData(pack_handler.m_segments[0]);
	encoder.WriteSegmentData(pack_handler.m_segments[1]);
	encoder.WriteSegmentData(pack_handler.m_segments[2]);
	encoder.WriteSegmentData(pack_handler.m_segments[3]);
	encoder.WriteSegmentData(pack_handler.m_segments[4]);
	encoder.WriteSegmentData(pack_handler.m_segments[5]);
	encoder.WriteSegmentData(pack_handler.m_segments[6]);
	encoder.WriteSegmentData(pack_handler.m_segments[7]);


	EXPECT_EQ(encode_handler.m_fec_frames.size(), 12);

	//
	// FEC decode
	//

	FecDecodeHandler decode_handler;
	FecDecoder decoder(&decode_handler);

	// lost the first original data
	decoder.WriteFecFrame(encode_handler.m_fec_frames[9]);
	decoder.WriteFecFrame(encode_handler.m_fec_frames[6]);
	decoder.WriteFecFrame(encode_handler.m_fec_frames[1]);
	decoder.WriteFecFrame(encode_handler.m_fec_frames[5]);
	decoder.WriteFecFrame(encode_handler.m_fec_frames[0]);
	decoder.WriteFecFrame(encode_handler.m_fec_frames[10]);
	decoder.WriteFecFrame(encode_handler.m_fec_frames[11]);
	decoder.WriteFecFrame(encode_handler.m_fec_frames[3]);

	EXPECT_EQ(decode_handler.m_segments.size(), 8);

	//
	// Unpack
	//

	UnpackHandler unpack_handler;
	VideoFrameUnpacker unpacker(&unpack_handler);

	for (const auto& segment : decode_handler.m_segments) {
		// FEC 解码后的数据仍然携带 FecHdr 头
		jukey::com::Buffer& buf = const_cast<jukey::com::Buffer&>(segment);
		buf.start_pos += FEC_HDR_LEN;
		buf.data_len -= FEC_HDR_LEN;
		unpacker.WriteSegmentData(segment);
	}

	EXPECT_EQ(unpack_handler.m_frames.size(), 1);

	VideoFrameHdr* hdr1 = (VideoFrameHdr*)DP(unpack_handler.m_frames[0]);

	EXPECT_EQ(hdr1->ver, hdr->ver);
	EXPECT_EQ(hdr1->codec, hdr->codec);
	EXPECT_EQ(hdr1->ext, hdr->ext);
	EXPECT_EQ(hdr1->h, hdr->h);
	EXPECT_EQ(hdr1->fseq, hdr->fseq);
	EXPECT_EQ(hdr1->sl, hdr->sl);
	EXPECT_EQ(hdr1->tl, hdr->tl);
	EXPECT_EQ(hdr1->ts, hdr->ts);
	EXPECT_EQ(hdr1->ft, hdr->ft);
}