#pragma once

#include <string>

#include "common-struct.h"
#include "common-enum.h"
#include "if-unknown.h"
#include "net-public.h"


namespace jukey::net
{

// Component ID and interface ID
#define CID_TCP_MGR "cid-tcp-mgr"
#define IID_TCP_MGR "iid-tcp-mgr"

//==============================================================================
// 
//==============================================================================
class ITcpHandler
{
public:
	//
	// @brief New tcp connection
	// @param lep listen endpoint
	// @param rep remote endpoint
	// @param sock socket ID
	//
	virtual void OnIncommingConn(const com::Endpoint& lep, 
		const com::Endpoint& rep,
		SocketId sock) = 0;

	//
	// @brief Result of connecting to server
	// @param rep remote endpoint
	// @param socket socket ID
	// @param result connect result, true:success,false:failed 
	//
	virtual void OnConnectResult(const com::Endpoint& lep, 
		const com::Endpoint& rep, 
		SocketId sock,
		bool result) = 0;

	//
	// @brief Notify connection closed
	// @param sock socket ID
	//
	virtual void OnConnClosed(const com::Endpoint& lep,
		const com::Endpoint& rep, 
		SocketId sock) = 0;

	//
	// @brief Notify received data from connection
	// @param sock socket ID
	// @param buf received data
	//
	virtual void OnRecvTcpData(const com::Endpoint& lep,
		const com::Endpoint& rep, 
		SocketId sock, 
		com::Buffer buf) = 0;
};

//==============================================================================
// 
//==============================================================================
class ITcpMgr : public base::IUnknown
{
public:
	//
	// @brief Initialize
	// @param handler event handler
	// @param thread_count multi-thread to process connection
	//
	virtual com::ErrCode Init(ITcpHandler* handler, uint32_t thread_count) = 0;

	//
	// @brief Start to listen
	// @param ep listen endpoint
	//
	virtual com::ErrCode AddListen(const com::Endpoint& ep) = 0;

	//
	// @brief Stop to listen
	// @param ep listen endpoint
	//
	virtual com::ErrCode RemoveListen(const com::Endpoint& ep) = 0;

	//
	// @brief Connect to server
	// @param ep server endpoint
	//
	virtual com::ErrCode Connect(const com::Endpoint& ep) = 0;

	//
	// @brief Send data
	// @param sock socket ID
	// @param buf data
	//
	virtual com::ErrCode SendData(SocketId sock, com::Buffer buf) = 0;

	//
	// @brief Close TCP connection
	// @param sock socket ID
	//
	virtual com::ErrCode CloseConn(SocketId sock) = 0;
};

}