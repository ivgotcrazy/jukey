#pragma once

#include <optional>

#include "common-struct.h"
#include "common-message.h"

#include "protoc/mq.pb.h"
#include "protoc/stream.pb.h"
#include "protoc/topo.pb.h"


namespace jukey::srv
{

//==============================================================================
// TODO: 
//==============================================================================
enum StreamMsgType
{
	STREAM_MSG_INVALID = SERVICE_STREAM_MSG_START + 0,
	STREAM_MSG_MQ_MSG  = SERVICE_STREAM_MSG_START + 1
};

typedef std::pair<prot::MqMsg, prot::Ping> ServicePingPair;
typedef std::optional<ServicePingPair> ServicePingPairOpt;

typedef std::pair<prot::MqMsg, prot::PublishStreamReq> PubStreamReqPair;
typedef std::optional<PubStreamReqPair> PubStreamReqPairOpt;

typedef std::pair<prot::MqMsg, prot::UnpublishStreamReq> UnpubStreamReqPair;
typedef std::optional<UnpubStreamReqPair> UnpubStreamReqPairOpt;

typedef std::pair<prot::MqMsg, prot::SubscribeStreamReq> SubStreamReqPair;
typedef std::optional<SubStreamReqPair> SubStreamReqPairOpt;

typedef std::pair<prot::MqMsg, prot::UnsubscribeStreamReq> UnsubStreamReqPair;
typedef std::optional<UnsubStreamReqPair> UnsubStreamReqPairOpt;

}