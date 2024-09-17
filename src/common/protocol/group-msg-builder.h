#pragma once

#include <string>

#include "common-struct.h"
#include "common-enum.h"

namespace jukey::prot::util
{
	
////////////////////////////////////////////////////////////////////////////////

struct MediaEntry
{
	std::string stream_id;
	uint32_t stream_type = 0;
	std::string media_src_id;
	uint32_t media_src_type = 0;
};

struct JoinGroupReqParam
{
	uint32_t app_id = 0;
	uint32_t group_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	uint32_t login_id = 0;
	std::vector<MediaEntry> media_entries;
	std::string token;
};

com::Buffer BuildJoinGroupReq(
	const JoinGroupReqParam& req_param,
	const com::SigHdrParam& hdr_param);

//------------------------------------------------------------------------------

struct MediaStateEntry
{
	MediaEntry media_entry;
	uint32_t media_state = 0;
};

struct GroupUserEntry
{
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	std::vector<MediaStateEntry> media_state_entries;
};

struct JoinGroupRspParam
{
	uint32_t app_id = 0;
	uint32_t group_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	uint32_t login_id = 0;
	std::vector<GroupUserEntry> group_users;
	com::ErrCode result = com::ERR_CODE_OK;
	std::string msg;
};

com::Buffer BuildJoinGroupRsp(
	const JoinGroupRspParam& rsp_param,
	const com::SigHdrParam& hdr_param);

//------------------------------------------------------------------------------

struct JoinGroupNotifyParam
{
	uint32_t app_id = 0;
	uint32_t group_id = 0;
	uint32_t user_type = 0;
	uint32_t user_id = 0;
	std::vector<MediaEntry> media_entries;
};

com::Buffer BuildJoinGroupNotify(
	const JoinGroupNotifyParam& notify_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct LeaveGroupReqParam
{
	uint32_t app_id = 0;
	uint32_t group_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	uint32_t login_id = 0;
	std::string token;
};

com::Buffer BuildLeaveGroupReq(
	const LeaveGroupReqParam& req_param,
	const com::SigHdrParam& hdr_param);

//------------------------------------------------------------------------------

struct LeaveGroupRspParam
{
	uint32_t app_id = 0;
	uint32_t group_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	uint32_t login_id = 0;
	com::ErrCode result = com::ERR_CODE_OK;
	std::string msg;
};

com::Buffer BuildLeaveGroupRsp(
	const LeaveGroupRspParam& rsp_param,
	const com::SigHdrParam& hdr_param);

//------------------------------------------------------------------------------

struct LeaveGroupNotifyParam
{
	uint32_t app_id = 0;
	uint32_t group_id = 0;
	uint32_t user_type = 0;
	uint32_t user_id = 0;
};

com::Buffer BuildLeaveGroupNotify(
	const LeaveGroupNotifyParam& notify_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct PubMediaReqParam
{
	uint32_t app_id = 0;
	uint32_t group_id = 0;
	uint32_t user_type = 0;
	uint32_t user_id = 0;
	uint32_t login_id = 0;
	MediaEntry media_entry;
};

com::Buffer BuildPubMediaReq(
	const PubMediaReqParam& req_param,
	const com::SigHdrParam& hdr_param);

//------------------------------------------------------------------------------

struct PubMediaRspParam
{
	uint32_t app_id = 0;
	uint32_t group_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	uint32_t login_id = 0;
	MediaEntry media_entry;
	uint32_t result = 0;
	std::string msg;
};

com::Buffer BuildPubMediaRsp(
	const PubMediaRspParam& rsp_param,
	const com::SigHdrParam& hdr_param);

//------------------------------------------------------------------------------

struct PubMediaNotifyParam
{
	uint32_t app_id = 0;
	uint32_t group_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	MediaEntry media_entry;
	bool need_ack = false;
};

com::Buffer BuildPubMediaNotify(
	const PubMediaNotifyParam& rsp_param,
	const com::SigHdrParam& hdr_param);

//------------------------------------------------------------------------------

struct PubMediaAckParam
{
	uint32_t app_id = 0;
	uint32_t group_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	MediaEntry media_entry;
	uint32_t ack_user_id = 0;
	uint32_t ack_user_type = 0;
};

com::Buffer BuildPubMediaAck(
	const PubMediaAckParam& ack_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

struct UnpubMediaReqParam
{
	uint32_t app_id = 0;
	uint32_t group_id = 0;
	uint32_t user_type = 0;
	uint32_t user_id = 0;
	uint32_t login_id = 0;
	MediaEntry media_entry;
};

com::Buffer BuildUnpubMediaReq(
	const UnpubMediaReqParam& req_param,
	const com::SigHdrParam& hdr_param);

//------------------------------------------------------------------------------

struct UnpubMediaRspParam
{
	uint32_t app_id = 0;
	uint32_t group_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	uint32_t login_id = 0;
	MediaEntry media_entry;
	uint32_t result = 0;
	std::string msg;
};

com::Buffer BuildUnpubMediaRsp(
	const UnpubMediaRspParam& rsp_param,
	const com::SigHdrParam& hdr_param);

//------------------------------------------------------------------------------

struct UnpubMediaNotifyParam
{
	uint32_t app_id = 0;
	uint32_t group_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	MediaEntry media_entry;
	bool need_ack = false;
};

com::Buffer BuildUnpubMediaNotify(
	const UnpubMediaNotifyParam& rsp_param,
	const com::SigHdrParam& hdr_param);

//------------------------------------------------------------------------------

struct UnpubMediaAckParam
{
	uint32_t app_id = 0;
	uint32_t group_id = 0;
	uint32_t user_id = 0;
	uint32_t user_type = 0;
	MediaEntry media_entry;
	uint32_t ack_user_id = 0;
	uint32_t ack_user_type = 0;
};

com::Buffer BuildUnpubMediaAck(
	const UnpubMediaAckParam& ack_param,
	const com::SigHdrParam& hdr_param);

////////////////////////////////////////////////////////////////////////////////

}