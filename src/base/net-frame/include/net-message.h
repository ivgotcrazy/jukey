#pragma once

#include <memory>

#include "common-struct.h"
#include "net-public.h"

namespace jukey::net
{

/*
 * Message type bits define 
 |---------------------------------------------------------------|
 |0                   1                   2                   3  |
 |0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1|
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |     module    |   sub-module  |          message type         |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * Module:
 *   0x01: core
 *   Sub-Module:
 *     0x01: net-frame
 *     
 *   0x02: service
 *   Sub-Module:
 *     0x01: proxy-service
 *     0x02: terminal-service
 *     0x03: user-service
 *     0x04: group-service
 *     0x05: stream-service
 *     0x06: stream-transport-service
*/

//==============================================================================
// TODO: reconnect
//==============================================================================
enum NetMsgType
{
	NET_MSG_INVALID                = 0x01010000 + 0,
	NET_MSG_SESSION_CLOSED         = 0x01010000 + 1,
	NET_MSG_SESSION_CREATE_RESULT  = 0x01010000 + 2,
	NET_MSG_SESSION_DATA           = 0x01010000 + 3,
	NET_MSG_SESSION_INCOMING       = 0x01010000 + 4,
	NET_MSG_SESSION_RESUME         = 0x01010000 + 5
};


//==============================================================================
// NET_MSG_SESSION_CLOSED
//==============================================================================
struct SessionClosedMsg
{
	SessionClosedMsg(SessionRole r, SessionId lsid, SessionId rsid)
		: role(r), lsid(lsid), rsid(rsid) {}

	SessionRole role = SessionRole::INVALID;
	SessionId lsid = INVALID_SESSION_ID;
	SessionId rsid = INVALID_SESSION_ID;
};
typedef std::shared_ptr<SessionClosedMsg> SessionClosedMsgSP;

//==============================================================================
// NET_MSG_SESSION_CREATE_RESULT
//==============================================================================
struct SessionCreateResultMsg
{
	SessionCreateResultMsg(SessionId lsid, SessionId rsid, bool res, 
		const com::Address& a)
		: lsid(lsid), rsid(rsid), result(res), addr(a) {}

	SessionId lsid = INVALID_SESSION_ID; // local session ID
	SessionId rsid = INVALID_SESSION_ID; // remote session ID
	bool result = false;
	com::Address addr;
};
typedef std::shared_ptr<SessionCreateResultMsg> SessionCreateResultMsgSP;

//==============================================================================
// NET_MSG_SESSION_DATA
//==============================================================================
struct SessionDataMsg
{
	SessionDataMsg(SessionRole r, SessionId lsid, SessionId rsid, 
		const com::Buffer& b) 
		: role(r), lsid(lsid), rsid(rsid), buf(b) {}

	SessionRole role = SessionRole::INVALID;
	SessionId lsid = INVALID_SESSION_ID;
	SessionId rsid = INVALID_SESSION_ID;
	com::Buffer buf;
};
typedef std::shared_ptr<SessionDataMsg> SessionDataMsgSP;

//==============================================================================
// NET_MSG_SESSION_INCOMING
//==============================================================================
struct SessionIncommingMsg
{
	SessionIncommingMsg(SessionId lsid, SessionId rsid, const com::Address& a)
		: lsid(lsid), rsid(rsid), addr(a) {}

	SessionId lsid = INVALID_SESSION_ID;
	SessionId rsid = INVALID_SESSION_ID;
	com::Address addr;
};
typedef std::shared_ptr<SessionIncommingMsg> SessionIncommingMsgSP;

//==============================================================================
// NET_MSG_SESSION_RESUME
//==============================================================================
struct SessionResumeMsg
{
	SessionResumeMsg(SessionId lsid, SessionId rsid) : lsid(lsid), rsid(rsid) {}

	SessionId lsid = INVALID_SESSION_ID;
	SessionId rsid = INVALID_SESSION_ID;
};
typedef std::shared_ptr<SessionResumeMsg> SessionResumeMsgSP;

}
