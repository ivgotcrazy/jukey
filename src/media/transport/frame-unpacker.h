#pragma once

#include <memory>
#include <map>
#include <set>
#include <unordered_set>

#include "common-struct.h"

namespace jukey::txp
{

//==============================================================================
// 
//==============================================================================
class IFrameUnpackHandler
{
public:
	virtual void OnFrameData(const com::Buffer& buf) = 0;
};

//==============================================================================
// 
//==============================================================================
class IFrameUnpacker
{
public:
	virtual void WriteSegmentData(const com::Buffer& buf) = 0;

	virtual uint32_t GetLossRate() = 0;
};
typedef std::unique_ptr<IFrameUnpacker> IFrameUnpackerUP;

//==============================================================================
// 
//==============================================================================
class AudioFrameUnpacker : public IFrameUnpacker
{
public:
	AudioFrameUnpacker(IFrameUnpackHandler* handler);
	~AudioFrameUnpacker();

	// IFrameUnpacker
	virtual void WriteSegmentData(const com::Buffer& buf) override;
	virtual uint32_t GetLossRate() override;

private:
	void PushSegment(const com::Buffer& buf);
	void TryPushCachedSegments();

private:
	IFrameUnpackHandler* m_handler = nullptr;
	std::mutex m_mutex;

	// TODO: 需要处理序号环回
	int64_t m_last_push_fseq = -1;

	std::map<uint32_t, com::Buffer> m_waiting_frames;

	uint32_t m_pushed_frames = 0;
	uint32_t m_missed_frames = 0;
};

//==============================================================================
// 
//==============================================================================
class VideoFrameUnpacker : public IFrameUnpacker
{
public:
	VideoFrameUnpacker(IFrameUnpackHandler* handler);
	~VideoFrameUnpacker();

	// IFrameUnpacker
	virtual void WriteSegmentData(const com::Buffer& buf) override;
	virtual uint32_t GetLossRate() override;

private:
	// key: sseq
	typedef std::map<uint32_t, com::Buffer> SegMap;

private:
	void TryAssembleVideoFrame();
	void PushFrame(const com::Buffer& buf, uint32_t fseq, uint32_t segs);
	void CheckFrameMapSize();

private:
	IFrameUnpackHandler* m_handler = nullptr;

	// key:fseq
	std::map<uint32_t, SegMap> m_frame_map;
	static const uint32_t KMaxCacheFrameSize = 32;

	// TODO: 需要处理序号环回
	uint32_t m_last_push_fseq = 0;

	std::set<uint32_t> m_stats_unpacked_frames;
	uint32_t m_unpack_good_seg = 0;
	uint32_t m_unpack_miss_seg = 0;

	// 平均每帧包含 segment 数量，用来估算丢失 segment 数量
	uint32_t m_segs_per_frame = 0;

	std::mutex m_mutex;
};


}