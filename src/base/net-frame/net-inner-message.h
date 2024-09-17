#pragma once

#include "net-common.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
enum NetInnerMsgType
{
	NET_INNER_MSG_INVALID           = 0x01010100 + 0,
	NET_INNER_MSG_TCP_CONNECT       = 0x01010100 + 1,
	NET_INNER_MSG_CONNECT_RESULT    = 0x01010100 + 2,
	NET_INNER_MSG_RECV_SESSION_DATA = 0x01010100 + 3,
	NET_INNER_MSG_ADD_SESSION       = 0x01010100 + 4,
	NET_INNER_MSG_REMOVE_SESSION    = 0x01010100 + 5,
	NET_INNER_MSG_SEND_SESSION_DATA = 0x01010100 + 6,
	NET_INNER_MSG_ALLOC_SESSION_ID  = 0x01010100 + 7
};

//==============================================================================
// NET_INNER_MSG_TCP_CONNECT
//==============================================================================
struct TcpConnectMsg
{
	TcpConnectMsg(const com::Endpoint& e, SocketId c) : ep(e), sock(c) {}

	com::Endpoint ep;
	SocketId sock;
};
typedef std::shared_ptr<TcpConnectMsg> TcpConnectMsgSP;

//==============================================================================
// NET_INNER_MSG_CONNECT_RESULT
//==============================================================================
struct ConnectResultMsg
{
	ConnectResultMsg(SessionId s, SocketId c, bool r)
		: sid(s), sock(c), result(r) {}

	SessionId sid;
	SocketId sock;
	bool result;
};
typedef std::shared_ptr<ConnectResultMsg> ConnectResultMsgSP;

//==============================================================================
// NET_INNER_MSG_RECV_SESSION_DATA
//==============================================================================
struct ConnectionDataMsg
{
	ConnectionDataMsg(SessionId id, const com::Buffer& b) : sid(id), buf(b) {}

	SessionId sid = 0;
	com::Buffer buf;
};
typedef std::shared_ptr<ConnectionDataMsg> ConnectionDataMsgSP;

//==============================================================================
// NET_INNER_MSG_SEND_SESSION_DATA
//==============================================================================
struct SendSessionDataMsg
{
	SendSessionDataMsg(SessionId id, const com::Buffer& b) : sid(id), buf(b) {}

	SessionId sid = 0;
	com::Buffer buf;
  std::promise<bool> result;
};
typedef std::shared_ptr<SendSessionDataMsg> SendSessionDataMsgSP;

//==============================================================================
// NET_INNER_MSG_REMOVE_SESSION
//==============================================================================
struct RemoveSessionMsg
{
	RemoveSessionMsg(SessionId id) : sid(id) {}
	RemoveSessionMsg(SessionId id, bool a) : sid(id), active(a) {}

	bool active = false;
	SessionId sid = 0;
};
typedef std::shared_ptr<RemoveSessionMsg> RemoveSessionMsgSP;

//==============================================================================
// MSG_TYPE_FETCH_SESSION_ID
//==============================================================================
struct FetchSessionIdMsg
{
	FetchSessionIdMsg(uint32_t tc, uint32_t ti)
		: thread_count(tc), thread_index(ti) {}

	uint32_t thread_count = 0;
	uint32_t thread_index = 0;
	std::promise<SessionId> sid;
};
typedef std::shared_ptr<FetchSessionIdMsg> FetchSessionIdMsgSP;

}
