#pragma once

#include <string>
#include "common-struct.h"
#include "common-enum.h"
#include "if-unknown.h"
#include "net-public.h"

namespace jukey::net
{

// Component ID and interface ID
#define CID_UDP_MGR "cid-udp-mgr"
#define IID_UDP_MGR "iid-udp-mgr"

//==============================================================================
// 
//==============================================================================
class IUdpHandler
{
public:
	//
	// @brief Notify received UDP data
	// @param sock which socket received data from
	// @param ep remote endpoint
	// @param buf received data
	//
	virtual void OnRecvUdpData(const com::Endpoint& lep, 
		const com::Endpoint& rep, 
		SocketId sock,
		com::Buffer buf) = 0;

	//
	// @brief Notify socket closed
	//
	virtual void OnSocketClosed(const com::Endpoint& lep,
		const com::Endpoint& rep,
		SocketId sock) = 0;
};

//==============================================================================
// 
//==============================================================================
class IUdpMgr : public base::IUnknown
{
public:
	//
	// @brief Initialize
	// @param handler event handler
	//
	virtual com::ErrCode Init(IUdpHandler* handler) = 0;

	//
	// @brief Create server socket with binding
	// @param ep local endpoint
	//
	virtual Socket CreateServerSocket(const com::Endpoint& ep) = 0;

	//
	// @brief Create client socket
	//
	virtual Socket CreateClientSocket() = 0;

	//
	// @brief Stop listen
	// @param sock socket to be closed
	//
	virtual void CloseSocket(Socket sock) = 0;

	//
	// @brief Send data
	// @param sock send data from
	// @param ep remote endpoint
	// @param buf data
	//
	virtual com::ErrCode SendData(Socket sock, const com::Endpoint& ep,
		com::Buffer buf) = 0;
};

}