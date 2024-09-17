#pragma once

#include <optional>

#include "common-struct.h"
#include "common-message.h"

#include "protoc/mq.pb.h"
#include "protoc/group.pb.h"
#include "protoc/stream.pb.h"

namespace jukey::srv
{

//==============================================================================
// 
//==============================================================================
enum GroupMsgType
{
	GROUP_MSG_INVALID = SERVICE_GROUP_MSG_START + 0,
	GROUP_MSG_MQ_MSG  = SERVICE_GROUP_MSG_START + 1
};

//==============================================================================
// 
//==============================================================================
typedef std::pair<prot::MqMsg, prot::JoinGroupReq> JoinGroupReqPair;
typedef std::optional<JoinGroupReqPair> JoinGroupReqPairOpt;

typedef std::pair<prot::MqMsg, prot::LeaveGroupReq> LeaveGroupReqPair;
typedef std::optional<LeaveGroupReqPair> LeaveGroupReqPairOpt;

typedef std::pair<prot::MqMsg, prot::PublishMediaReq> PubMediaReqPair;
typedef std::optional<PubMediaReqPair> PubMediaReqPairOpt;

typedef std::pair<prot::MqMsg, prot::UnpublishMediaReq> UnpubMediaReqPair;
typedef std::optional<UnpubMediaReqPair> UnpubMediaReqPairOpt;

//==============================================================================
// 
//==============================================================================
struct MediaEntry
{
	com::StreamType stream_type = com::StreamType::INVALID;
	std::string stream_id;
	com::MediaSrcType media_src_type = com::MediaSrcType::INVALID;
	std::string media_src_id;
	uint32_t state = 0;
};

//==============================================================================
// 
//==============================================================================
struct UserEntry
{
	uint32_t user_type;
	uint32_t user_id;
	std::vector<MediaEntry> medias;
};

//==============================================================================
// 
//==============================================================================
struct GroupEntry
{
	// {UserID:UserEntry}
	std::map<uint32_t, UserEntry> users;
};

//==============================================================================
// 
//==============================================================================
struct AppEntry
{
	// {GroupID:GroupEntry}
	std::map<uint32_t, GroupEntry> groups;
};

}