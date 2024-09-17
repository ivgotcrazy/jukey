#include "gtest/gtest.h"
#include "frame-packer.h"
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

TEST(FramePack, audio_normal)
{
	PackHandler handler;
	FramePacker packer(StreamType::AUDIO, &handler);

	Buffer buf(320, 320);
	
	AudioFrameHdr* hdr = (AudioFrameHdr*)(buf.data.get());
	hdr->ver = 0;
	hdr->chnls = 2;
	hdr->codec = 1;
	hdr->ext = 0;
	hdr->power = 10;
	hdr->fseq = 100;
	hdr->srate = 1;
	hdr->ts = 100;

	packer.WriteFrameData(buf);
	
	EXPECT_EQ(handler.m_buffers.size(), 1);
	EXPECT_EQ(handler.m_buffers.front().data_len, 320 + sizeof(SegHdr));
}

TEST(FramePack, audio_invalid_length)
{
	PackHandler handler;
	FramePacker packer(StreamType::AUDIO, &handler);

	Buffer buf(1025, 1025);

	AudioFrameHdr* hdr = (AudioFrameHdr*)(buf.data.get());
	hdr->ver = 0;
	hdr->chnls = 2;
	hdr->codec = 1;
	hdr->ext = 0;
	hdr->power = 10;
	hdr->fseq = 100;
	hdr->srate = 1;
	hdr->ts = 100;

	packer.WriteFrameData(buf);

	EXPECT_EQ(handler.m_buffers.size(), 0);
}

TEST(FramePack, video_normal)
{
	PackHandler handler;
	FramePacker packer(StreamType::VIDEO, &handler);

	Buffer buf(2000, 2000);

	VideoFrameHdr* hdr = (VideoFrameHdr*)(buf.data.get());
	hdr->ver = 0;
	hdr->codec = 1;
	hdr->ext = 0;
	hdr->h = 1;
	hdr->fseq = 100;
	hdr->sl = 0;
	hdr->tl = 0;
	hdr->ts = 100;
	hdr->ft = 1;

	packer.WriteFrameData(buf);

	EXPECT_EQ(handler.m_buffers.size(), 2);
	EXPECT_EQ(handler.m_buffers[0].data_len, 1024 + sizeof(SegHdr));
	EXPECT_EQ(handler.m_buffers[1].data_len, 1024 + sizeof(SegHdr));

	SegHdr* hdr1 = (SegHdr*)(handler.m_buffers[0].data.get());
	EXPECT_EQ(hdr1->fseq, 100);
	EXPECT_EQ(hdr1->slen, 1040);
	EXPECT_EQ(hdr1->sseq, 0);
	EXPECT_EQ(hdr1->st, 0);

	SegHdr* hdr2 = (SegHdr*)(handler.m_buffers[1].data.get());
	EXPECT_EQ(hdr2->fseq, 100);
	EXPECT_EQ(hdr2->slen, 992);
	EXPECT_EQ(hdr2->sseq, 1);
	EXPECT_EQ(hdr2->st, 1);
}