#include "join-processor.h"
#include "log.h"
#include "rtc-common.h"
#include "rtc-engine-impl.h"
#include "protoc/group.pb.h"
#include "group-msg-builder.h"
#include "common/util-pb.h"
#include "util-protocol.h"

using namespace jukey::com;

namespace jukey::sdk
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
JoinProcessor::JoinProcessor(RtcEngineImpl* engine) : m_rtc_engine(engine)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void JoinProcessor::OnJoinGroupRsp(const com::Buffer& buf)
{
	prot::JoinGroupRsp rsp;
	if (!rsp.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse join group response failed!");
		G(m_engine_state) = RtcEngineState::LOGINED;
		G(m_engine_param).handler->OnJoinGroupResult(G(m_engine_data.group_id),
			ERR_CODE_FAILED);
		return;
	}

	LOG_INF("Received join group response:{}", util::PbMsgToJson(rsp));

	if (rsp.result() == (uint32_t)ERR_CODE_OK) {
		LOG_INF("Join group success");
		G(m_engine_state) = RtcEngineState::JOINED;
		G(m_engine_param).handler->OnJoinGroupResult(G(m_engine_data.group_id),
			ERR_CODE_OK);
	}
	else {
		LOG_ERR("Join group failed");
		G(m_engine_state) = RtcEngineState::LOGINED;
		G(m_engine_param).handler->OnJoinGroupResult(G(m_engine_data.group_id),
			ERR_CODE_FAILED);
	}

	for (int i = 0; i < rsp.group_users_size(); i++) {
		prot::GroupUser user = rsp.group_users(i);
		for (int j = 0; j < user.media_state_entries_size(); j++) {
			prot::MediaStateEntry state_entry = user.media_state_entries(j);
			if (state_entry.media_state() == 1) {
				const prot::MediaEntry& media_entry = state_entry.media_entry();

				LOG_INF("Group user publish media, user:{}, media:{}|{}, stream:{}|{}",
					user.user_id(),
					media_entry.media_src_type(),
					media_entry.media_src_id(),
					media_entry.stream_type(),
					media_entry.stream_id());

				com::MediaStream stream;
				stream = prot::util::ToMediaStream(media_entry);
				stream.src.app_id = rsp.app_id();
				stream.src.user_id = user.user_id();

				G(m_engine_param).handler->OnPubStreamNotify(rsp.group_id(), stream, true);
			}
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void JoinProcessor::OnJoinGroupTimeout()
{
	G(m_engine_state) = RtcEngineState::LOGINED;
	G(m_engine_param).handler->OnJoinGroupResult(G(m_engine_data.group_id),
		ERR_CODE_TIMEOUT);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void JoinProcessor::OnJoinGroupError(ErrCode ec)
{
	G(m_engine_state) = RtcEngineState::LOGINED;
	G(m_engine_param).handler->OnJoinGroupResult(G(m_engine_data.group_id),
		ERR_CODE_FAILED);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode JoinProcessor::JoinGroup(uint32_t group_id)
{
	if (group_id == 0) {
		LOG_ERR("Invalid group ID!");
		return ERR_CODE_INVALID_PARAM;
	}

	std::lock_guard<std::recursive_mutex> lock(G(m_mutex));

	if (G(m_engine_state) != RtcEngineState::LOGINED) {
		LOG_ERR("Invalid state:{} to join group!", G(m_engine_state));
		return ERR_CODE_FAILED;
	}

	IMediaDevMgr* dev_mgr = G(m_media_engine)->GetMediaDevMgr();
	std::vector<CamDevice> cameras = dev_mgr->GetCamDevList();
	std::vector<MicDevice> microphones = dev_mgr->GetMicDevList();

	Buffer buf = G(m_msg_builder)->BuildJoinGroupReq(group_id,
		++G(m_cur_seq), cameras, microphones);

	G(m_async_proxy)->SendSessionMsg(G(m_proxy_session), buf, G(m_cur_seq),
		prot::MSG_JOIN_GROUP_RSP)
		.OnResponse([this](net::SessionId sid, const com::Buffer& buf) {
			OnJoinGroupRsp(buf);
		})
		.OnTimeout([this]() {
			OnJoinGroupTimeout();
		})
		.OnError([this](const std::string& err) {
			OnJoinGroupError(ERR_CODE_FAILED);
		});
		
	LOG_INF("Send join group request, seq:{}, app:{}, group:{}, user:{}, client:{}",
		G(m_cur_seq), 
		G(m_engine_param).app_id, 
		group_id, 
		G(m_engine_data.user_id),
		G(m_engine_param).client_id);

	G(m_engine_data.group_id) = group_id;
	G(m_engine_state) = RtcEngineState::JOINING;

	return ERR_CODE_OK;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void JoinProcessor::OnLeaveGroupRsp(const com::Buffer& buf)
{
	G(m_engine_state) = RtcEngineState::LOGINED;

	prot::LeaveGroupRsp rsp;
	if (!rsp.ParseFromArray(PB_PARSE_SIG_PARAM(buf))) {
		LOG_ERR("Parse leave group response failed!");
		G(m_engine_param).handler->OnLeaveGroupResult(
			G(m_engine_data.group_id), ERR_CODE_FAILED);
	}
	else {
		LOG_INF("Received join group response:{}", util::PbMsgToJson(rsp));

		if (rsp.result() == (uint32_t)ERR_CODE_OK) {
			LOG_INF("Leave group success");
			G(m_engine_param).handler->OnLeaveGroupResult(
				G(m_engine_data.group_id), ERR_CODE_OK);
		}
		else {
			LOG_ERR("Leave group failed");
			G(m_engine_param).handler->OnJoinGroupResult(
				G(m_engine_data.group_id),ERR_CODE_FAILED);
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void JoinProcessor::OnLeaveGroupTimeout()
{
	G(m_engine_state) = RtcEngineState::LOGINED;
	G(m_engine_param).handler->OnLeaveGroupResult(G(m_engine_data.group_id),
		ERR_CODE_TIMEOUT);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void JoinProcessor::OnLeaveGroupError(ErrCode ec)
{
	G(m_engine_state) = RtcEngineState::LOGINED;
	G(m_engine_param).handler->OnLeaveGroupResult(G(m_engine_data.group_id),
		ERR_CODE_FAILED);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ErrCode JoinProcessor::LeaveGroup()
{
	std::lock_guard<std::recursive_mutex> lock(G(m_mutex));

	if (G(m_engine_state) != RtcEngineState::JOINED) {
		LOG_ERR("Invalid state:{} to leave group!", G(m_engine_state));
		return ERR_CODE_FAILED;
	}

	Buffer buf = G(m_msg_builder)->BuildLeaveGroupReq(++G(m_cur_seq));

	G(m_async_proxy)->SendSessionMsg(G(m_proxy_session), buf, G(m_cur_seq),
		prot::MSG_LEAVE_GROUP_RSP)
		.OnResponse([this](net::SessionId sid, const com::Buffer& buf) {
			OnLeaveGroupRsp(buf);
		})
		.OnTimeout([this]() {
			OnLeaveGroupTimeout();
		})
		.OnError([this](const std::string& err) {
			OnLeaveGroupError(ERR_CODE_FAILED);
		});

	return ERR_CODE_OK;
}

}