#pragma once

#include "common-struct.h"
#include "if-rtc-engine.h"
#include "rtc-common.h"

#include "protoc/stream.pb.h"

namespace jukey::sdk
{

//==============================================================================
// 
//==============================================================================
class MsgBuilder
{
public:
	MsgBuilder(const RtcEngineParam& param, const RtcEngineData& data);

	com::Buffer BuildPubStreamReq(const com::MediaStream& stream, uint32_t seq);
	com::Buffer BuildUnpubStreamReq(const com::MediaStream& stream, uint32_t seq);

	com::Buffer BuildPubMediaReq(const com::MediaStream& stream, uint32_t seq);
	com::Buffer BuildUnpubMediaReq(const com::MediaStream& stream, uint32_t seq);

	com::Buffer BuildLoginSendChannelAck(const com::Buffer& buf,
		const prot::LoginSendChannelNotify& notify);

	com::Buffer BuildSubStreamReq(const com::MediaStream& stream, uint32_t seq);

	com::Buffer BuildUnsubStreamReq(const com::MediaStream& stream, uint32_t seq);

	com::Buffer BuildUserLoginReq(uint32_t user_id, uint32_t seq);

	com::Buffer BuildUserLogoutReq(uint32_t seq);

	com::Buffer BuildJoinGroupReq(uint32_t group_id, uint32_t seq, 
		const std::vector<CamDevice>& cameras,
		const std::vector<MicDevice>& microphones);

	com::Buffer BuildLeaveGroupReq(uint32_t seq);

	com::Buffer BuildClientRegReq(uint32_t seq);

private:
	const RtcEngineData& m_engine_data;
	const RtcEngineParam& m_engine_param;
};
typedef std::unique_ptr<MsgBuilder> MsgBuilderUP;

}