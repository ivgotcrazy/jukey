#pragma once

#include "common-struct.h"
#include "session-protocol.h"
#include "if-session-pkt-assembler.h"

namespace jukey::net
{

//==============================================================================
// 
//==============================================================================
class TcpSessionPktAssembler : public ISessionPktAssembler
{
public:
	TcpSessionPktAssembler(SessionId local_sid);
	~TcpSessionPktAssembler();

	// ISessionPktAssembler
	virtual void InputSessionData(const com::Buffer& buf) override;
	virtual SessionPktSP GetNextSessionPkt() override;

private:
	bool HasEnoughSpaceForIncommingBuf(const com::Buffer& buf);
	void ParseDataWithNoBuffer(const com::Buffer& buf);
	void ParseDataWithBuffer(const com::Buffer& buf);
	void MakeSessionPkt(char** data, uint32_t* len);

private:
	SessionId m_local_sid = INVALID_SESSION_ID;

	// 接收缓冲区
	char* m_recv_buf = nullptr;
	uint32_t m_data_pos = 0;

	// 接收缓冲区最大长度
	uint32_t m_max_len = 1024 * 128;

	// 接收缓冲区数据长度
	uint32_t m_data_len = 0;

	std::list<SessionPktSP> m_pkt_list;
};

}