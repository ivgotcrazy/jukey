#include "gtest/gtest.h"
#include "fec-encoder.h"
#include "protocol.h"

using namespace jukey::com;
using namespace jukey::txp;
using namespace jukey::prot;

class FecEncodeHandler : public IFecEncodeHandler
{
public:
	virtual void OnFecFrameData(const Buffer& buf) override
	{
		m_fec_frames.push_back(buf);
	}

	std::vector<Buffer> m_fec_frames;
};

TEST(FecEncode, Normal)
{
	SeqAllocator allocator;
	FecEncodeHandler handler;
	FecEncoder encoder(&handler, &allocator);

	encoder.SetParam(16, 4);

	Buffer seg(512, 512);

	for (auto i = 0; i < 16; ++i) {
		encoder.WriteSegmentData(seg);
	}

	EXPECT_EQ(handler.m_fec_frames.size(), 20);

	FecHdr* hdr1 = (FecHdr*)DP(handler.m_fec_frames[0]);
	EXPECT_EQ(hdr1->K, 15);
	EXPECT_EQ(hdr1->R, 4);
	EXPECT_EQ(hdr1->group, 0);
	EXPECT_EQ(hdr1->gseq, 0);
	EXPECT_EQ(hdr1->seq, 0);

	FecHdr* hdr2 = (FecHdr*)DP(handler.m_fec_frames[1]);
	EXPECT_EQ(hdr2->K, 15);
	EXPECT_EQ(hdr2->R, 4);
	EXPECT_EQ(hdr2->group, 0);
	EXPECT_EQ(hdr2->gseq, 1);
	EXPECT_EQ(hdr2->gseq, 1);

	FecHdr* hdr3 = (FecHdr*)DP(handler.m_fec_frames[2]);
	EXPECT_EQ(hdr3->K, 15);
	EXPECT_EQ(hdr3->R, 4);
	EXPECT_EQ(hdr3->group, 0);
	EXPECT_EQ(hdr3->gseq, 2);
	EXPECT_EQ(hdr3->gseq, 2);
}