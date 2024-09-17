#pragma once

#include <set>
#include <map>

#include "common-struct.h"
#include "fec/if-fec-decoder.h"
#include "protocol.h"

namespace jukey::txp
{

//==============================================================================
// 
//==============================================================================
class IFecDecodeHandler
{
public:
	virtual void OnSegmentData(const com::Buffer& buf) = 0;
};

//==============================================================================
// 
//==============================================================================
class FecDecoder
{
public:
	FecDecoder(IFecDecodeHandler* handler);
	~FecDecoder();

	void WriteFecFrame(const com::Buffer& buf);

	uint32_t GetFecLossRate();

private:
	typedef std::map<uint32_t, com::Buffer> FecGroupMap;

private:
	bool UpdateFecParam(uint8_t k, uint8_t r);
	void InsertFecFrame(const com::Buffer& buf);

	void TryFecDecodeOutOfOrder();
	void TryFecDecodeInOrder();

	void ProcWithDecode(const com::Buffer& buf);
	void ProcWithoutDecode(const com::Buffer& buf);

	void UpdatePushedGroup(uint32_t group);

	void DoFecDecode(uint32_t group, const FecGroupMap& frame_map);

	void PushGroupWithDecode(uint32_t group, const FecGroupMap& frame_map);
	void PushGroupWithoutDecode(uint32_t group, const FecGroupMap& frame_map);

	void RefreshFecGroups();
	void FlushFecGroup(uint32_t group, const FecGroupMap& frame_map);

	void InitMinPushHdr();
	void UpdateMinPushHdr();

	void PushSegment(const com::Buffer& buf, bool stats);

private:
	IFecDecodeHandler* m_handler = nullptr;
	util::IFecDecodeUP m_decoder;

	uint8_t m_k = 0;
	uint8_t m_r = 0;

	// 报文可能乱序到达，需要支持同时解码多个 group
	//std::map<uint32_t, std::list<com::Buffer>> m_fec_groups;
	std::map<uint32_t, FecGroupMap> m_fec_groups;
	static const uint32_t kMaxFecGroupCacheSize = 8;

	uint32_t m_data_len = 0;

	// 回调出去的 segment 数量
	uint32_t m_push_seg_count = 0;

	//////////// K != 0 丢包统计相关
	// 记录统计周期内已经 push 的 group，用来分析整个 group 丢失
	std::set<uint32_t> m_stats_pushed_groups;
	uint32_t m_push_group_count = 0;

	//////////// K = 0 丢包统计相关
	int64_t m_min_input_seq = -1;
	int64_t m_max_input_seq = -1;

	std::mutex m_mutex;

	int64_t m_last_push_group = -1;
};

}