#include "gtest/gtest.h"
#include "frame-packer.h"
#include "frame-unpacker.h"
#include "common-struct.h"
#include "protocol.h"

using namespace jukey::com;
using namespace jukey::txp;
using namespace jukey::prot;

class PackHandler : public IFramePackHandler
{
public:
	void OnSegmentData(const Buffer& buf) override
	{
		m_buffers.push_back(buf);
	}

	std::vector<Buffer> m_buffers;
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

TEST(FrameUnpack, audio_normal)
{
	//
	// Pack
	//

	PackHandler pack_handler;
	FramePacker packer(StreamType::AUDIO, &pack_handler);

	Buffer frame_buf(320, 320);
	
	AudioFrameHdr* hdr = (AudioFrameHdr*)(frame_buf.data.get());
	hdr->ver = 0;
	hdr->chnls = 2;
	hdr->codec = 1;
	hdr->ext = 0;
	hdr->power = 10;
	hdr->fseq = 100;
	hdr->srate = 1;
	hdr->ts = 100;

	packer.WriteFrameData(frame_buf);
	
	//
	// Unpack
	//

	UnpackHandler unpack_handler;
	AudioFrameUnpacker unpacker(&unpack_handler);

	unpacker.WriteSegmentData(pack_handler.m_buffers.front());

	EXPECT_EQ(unpack_handler.m_frames.size(), 1);
	EXPECT_EQ(frame_buf.data_len, unpack_handler.m_frames.front().data_len);
	EXPECT_EQ(memcmp(DP(frame_buf), DP(unpack_handler.m_frames.front()), frame_buf.data_len), 0);
}


TEST(FrameUnpack, video_normal)
{
	//
	// Pack
	//

	PackHandler pack_handler;
	FramePacker packer(StreamType::VIDEO, &pack_handler);

	Buffer frame_buf(2000, 2000);

	VideoFrameHdr* hdr = (VideoFrameHdr*)(frame_buf.data.get());
	hdr->ver = 0;
	hdr->codec = 1;
	hdr->ext = 0;
	hdr->w = 1;
	hdr->h = 1;
	hdr->ft = 1;
	hdr->sl = 0;
	hdr->tl = 0;
	hdr->fseq = 100;
	hdr->ts = 100;
	
	packer.WriteFrameData(frame_buf);

	//
	// Unpack
	//
	
	UnpackHandler unpack_handler;
	VideoFrameUnpacker unpacker(&unpack_handler);

	for (const auto& segment : pack_handler.m_buffers) {
		unpacker.WriteSegmentData(segment);
	}
	
	EXPECT_EQ(unpack_handler.m_frames.size(), 1);
	EXPECT_EQ(frame_buf.data_len, unpack_handler.m_frames.front().data_len);
	EXPECT_EQ(memcmp(DP(frame_buf), DP(unpack_handler.m_frames.front()), frame_buf.data_len), 0);
}