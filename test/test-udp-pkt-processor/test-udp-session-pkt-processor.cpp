// test-udp-session-pkt-processor.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

#include "if-session.h"
#include "reliable-udp-session-pkt-processor.h"
#include "component-factory.h"
#include "if-udp-mgr.h"
#include "proxy-unknown.h"
#include "clipp.h"
#include "common/util-common.h"

using namespace jukey::util;
using namespace jukey::com;
using namespace jukey::net;
using namespace jukey::base;

using namespace clipp;

class MyUdpMgr : public IUdpMgr, public ProxyUnknown
{
public:
	MyUdpMgr(IComFactory* factory) : ProxyUnknown(nullptr)
	{

	}

	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	virtual ErrCode Init(IUdpHandler* handler) override
	{
		return ErrCode::ERR_CODE_OK;
	}

	virtual Socket CreateServerSocket(const Endpoint& ep) override
	{
		return 0;
	}

	virtual Socket CreateClientSocket() override
	{
		return 0;
	}

	virtual void CloseSocket(Socket sock) override
	{

	}

	virtual ErrCode SendData(Socket sock, const Endpoint& ep, Buffer buf) override
	{
		return ErrCode::ERR_CODE_OK;
	}
};

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
IUnknown* MyUdpMgr::CreateInstance(IComFactory* factory,
	const char* cid)
{
	if (strcmp(cid, CID_UDP_MGR) == 0) {
		return new MyUdpMgr(factory);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* MyUdpMgr::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_UDP_MGR)) {
		return static_cast<IUdpMgr*>(this);
	}
	else {
		return ProxyUnknown::NDQueryInterface(riid);
	}
}

class MySession : public ISession
{
public:
	virtual void Init() override
	{

	}

	virtual void OnTimeout(uint32_t msg_type) override
	{

	}

	virtual void OnRecvData(const Buffer& buf) override
	{

	}

	virtual void SendData(const Buffer& buf) override
	{

	}

	virtual void Close() override
	{

	}

	virtual Endpoint GetEndpoint() override
	{
		return Endpoint();
	}

	virtual ConnId GetConnId() override
	{
		return 0;
	}

	virtual SessionId GetLocalSid() override
	{
		return 0;
	}

	virtual SessionId GetRemoteSid() override
	{
		return 0;
	}
};

int main(int argc, char** argv)
{
	uint32_t size = 1024;
	uint32_t count = 0;
	uint32_t rate = 1000;

	auto cli = (
		option("-s", "--size") & value("packet size", size),
		option("-c", "--count") & value("total count", count),
		option("-r", "--rate") & value("count per second", rate)
		);

	if (!parse(argc, argv, cli)) {
		std::cout << make_man_page(cli, argv[0]);
		return -1;
	}

	std::cout << "size:" << size << ", count:" << count << ", rate:" << rate << std::endl;

	uint32_t sleep_interval = 1; // ms
	uint32_t loop_count = (rate + 999) / 1000;

	std::cout << "loop count:" << loop_count << std::endl;

	IComFactory* factory = GetComFactory();
    IUdpMgr* udp_mgr = new MyUdpMgr(factory);
	ISession* session = new MySession();

    ReliableUdpSessionPktProcessor* processor = new ReliableUdpSessionPktProcessor(factory, udp_mgr, session);

	Buffer buf(sizeof(SessPktHead) + size);
	buf.data_len = sizeof(SessPktHead) + size;
	
	SessPktHead* head = (SessPktHead*)(buf.data.get());
	head->ver = 1;
	head->type = SESSION_PACKET_DATA;
	head->rsv = 0;
	head->wnd = 2048;
	head->frag = 0;
	head->len = size;
	head->src = 100;
	head->dst = 101;
	head->sn = 0;
	head->ts = static_cast<uint32_t>(Now());
	
	SessionPktSP pkt;
	uint32_t pkt_count = 0;

	for (auto i = 0; i < count; i++) {

		for (auto j = 0; j < loop_count; j++) {
			head->sn++;
			processor->SendSessionData(buf);
		}
		
		while (pkt = processor->GetAvaliableSessionPkt()) {
			//std::cout << "###" << ++pkt_count << std::endl;
		}
	}
	
	return 0;
}
