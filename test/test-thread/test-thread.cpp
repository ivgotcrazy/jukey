// test-common-thread.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <unordered_map>

#include "thread/common-thread.h"
#include "thread/concurrent-thread.h"
#include "common/util-time.h"
#include "common/util-stats.h"
#include "com-factory.h"
#include "clipp.h"

using namespace jukey::util;
using namespace jukey::base;
using namespace jukey::com;

using namespace clipp;


SpdlogWrapperSP g_logger = std::make_shared<SpdlogWrapper>("test");

//==============================================================================
// 
//==============================================================================
class ITest
{
public:
	virtual void Start() = 0;
		virtual void Stop() = 0;
	virtual void AddMsg(const CommonMsg& msg) = 0;
};

//==============================================================================
// 
//==============================================================================
class TestCommonThread : public ITest, public CommonThread
{
public:
	TestCommonThread() : CommonThread("test", 4096, false)
	{
		IComFactory* factory = GetComFactory();
		factory->Init("./");

		m_data_stats.reset(new DataStats(factory, g_logger, "test"));
		m_data_stats->Start();

		StatsParam recv_stats("recv msg count",
			StatsType::IACCU, 5000);
		m_recv_count_stats = m_data_stats->AddStats(recv_stats);

		StatsParam send_stats("send msg count",
			StatsType::IACCU, 5000);
		m_send_count_stats = m_data_stats->AddStats(send_stats);
	}

	virtual void Start() override
	{
		m_map.insert(std::make_pair(10, 10));
		StartThread();
	}

	virtual void Stop() override
	{
		StopThread();
		m_data_stats->Stop();
	}

	virtual void OnThreadMsg(const CommonMsg& msg) override
	{
		auto iter = m_map.find(10);
		if (iter != m_map.end()) {
			m_data_stats->OnData(m_recv_count_stats, 1);
		}   
	}

	virtual void AddMsg(const CommonMsg& msg) override
	{
		PostMsg(msg);
		m_data_stats->OnData(m_send_count_stats, 1);
	}

private:
	DataStatsSP m_data_stats;
	StatsId m_recv_count_stats = 0;
	StatsId m_send_count_stats = 0;
	std::unordered_map<uint16_t, uint16_t> m_map;
};

//==============================================================================
// 
//==============================================================================
class TestConcurrentThread : public ITest, public ConcurrentThread
{
public:
	TestConcurrentThread() : ConcurrentThread("test", 4096)
	{
		IComFactory* factory = GetComFactory();
		factory->Init("./");

		m_data_stats.reset(new DataStats(factory, g_logger, "test"));
		m_data_stats->Start();

		StatsParam recv_stats("recv msg count",
			StatsType::IACCU, 5000);
		m_recv_count_stats = m_data_stats->AddStats(recv_stats);

		StatsParam send_stats("send msg count",
			StatsType::IACCU, 5000);
		m_send_count_stats = m_data_stats->AddStats(send_stats);
	}

	virtual void Start() override
	{
		m_map.insert(std::make_pair(10, 10));
		StartThread();
	}

	virtual void Stop() override
	{
		StopThread();
		m_data_stats->Stop();
	}

	virtual void OnThreadMsg(const CommonMsg& msg) override
	{
		auto iter = m_map.find(10);
		if (iter != m_map.end()) {
			m_data_stats->OnData(m_recv_count_stats, 1);
		}
	}

	virtual void AddMsg(const CommonMsg& msg) override
	{
		PostMsg(msg);
		m_data_stats->OnData(m_send_count_stats, 1);
	}

private:
	DataStatsSP m_data_stats;
	StatsId m_recv_count_stats = 0;
	StatsId m_send_count_stats = 0;
	std::unordered_map<uint16_t, uint16_t> m_map;
};

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
int main(int argc, char** argv)
{
	uint32_t size = 1024;
	uint32_t count = 1000000;
	uint32_t rate = 1000;
	uint32_t type = 1;

	auto cli = (
		option("-s", "--size") & value("packet size", size),
		option("-c", "--count") & value("total count", count),
		option("-r", "--rate") & value("count per second", rate),
		required("-t", "--type") & value("1: common thread, 2: concurrent thread", type)
	);

	if (!parse(argc, argv, cli)) {
		std::cout << make_man_page(cli, argv[0]);
		return -1;
	}

	std::cout << "size:" << size << ", count:" << count << ", rate:" << rate << std::endl;

	uint32_t sleep_interval = 1; // ms
	uint32_t loop_count = (rate + 999) / 1000;

	std::cout << "loop count:" << loop_count << std::endl;

	CommonMsg msg;
	msg.msg_type = 100;
	msg.msg_data.reset(new uint8_t[size]);

	ITest* test = nullptr;
	if (type == 1) {
		test = new TestCommonThread();
	}
	else if (type == 2) {
		test = new TestConcurrentThread();
	}
	else {
		std::cout << "Invalid thread type:" << type << std::endl;
		return -1;
	}

	std::cout << "Test begin" << std::endl;

	test->Start();

	uint32_t send_count = 0;
	while (count == 0 || send_count <= count) {
		jukey::util::Sleep(sleep_interval);
		for (uint32_t i = 0; i < loop_count; i++) {
			test->AddMsg(msg);
			++send_count;
			if (send_count >= count) break;
		}
	}

	test->Stop();

	std::cout << "Test end" << std::endl;

	return 0;
}
