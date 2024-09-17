#include <iostream>

#include "if-amqp-client.h"
#include "com-factory.h"
#include "common-struct.h"
#include "common/util-time.h"

using namespace jukey::base;
using namespace jukey::util;
using namespace jukey::com;


class AmqpProducer : public IAmqpHandler
{
public:
	AmqpProducer(IComFactory* factory) : m_factory(factory) {}

	bool Init()
	{
		m_amqp_client = (IAmqpClient*)m_factory->QueryInterface(CID_AMQP_CLIENT,
			IID_AMQP_CLIENT, "test");
		if (!m_amqp_client) {
			std::cout << "Create amqp client failed!" << std::endl;
			return true;
		}

		AmqpParam param;
		param.host = "192.168.79.131";
		param.port = 5672;
		param.user = "guest";
		param.pwd = "guest";
		param.handler = this;
		if (ErrCode::ERR_CODE_OK != m_amqp_client->Init(param)) {
			std::cout << "Initialize amqp client failed!" << std::endl;
			return false;
		}

		m_amqp_client->DeclareExchange("test-fanout-exchange", ExchangeType::FANOUT);

		return true;
	}

	virtual void OnRecvMqMsg(const std::string& queue, const Buffer& mq_buf, const Buffer& sig_buf) override
	{
		//std::cout << queue << ":" << std::string((char*)buf.data.get(), buf.data_len) << std::endl;
	}

	void PublishMsg()
	{
		for (auto i = 0; i < 128; i++) {
			jukey::com::Buffer buf("hello world");
			m_amqp_client->Publish("test-fanout-exchange", "", buf, i);
		}
	}

private:
	IComFactory* m_factory = nullptr;
	IAmqpClient* m_amqp_client = nullptr;
};


class AmqpConsumer : public IAmqpHandler
{
public:
	AmqpConsumer(IComFactory* factory, const std::string& name, uint32_t sleep) 
		: m_factory(factory), m_consumer_name(name), m_sleep_time(sleep)
	{
	}

	bool Init()
	{
		m_amqp_client = (IAmqpClient*)m_factory->QueryInterface(CID_AMQP_CLIENT,
			IID_AMQP_CLIENT, "test");
		if (!m_amqp_client) {
			std::cout << "Create amqp client failed!" << std::endl;
			return true;
		}

		AmqpParam param;
		param.host = "192.168.79.131";
		param.port = 5672;
		param.user = "guest";
		param.pwd = "guest";
		param.handler = this;
		if (ErrCode::ERR_CODE_OK != m_amqp_client->Init(param)) {
			std::cout << "Initialize amqp client failed!" << std::endl;
			return false;
		}

		m_amqp_client->DeclareQueue("test-fanout-queue");
		m_amqp_client->BindQueue("test-fanout-exchange", "test-fanout-queue", "");

		return true;
	}

	virtual void OnRecvMqMsg(const std::string& queue, const Buffer& mq_buf, const Buffer& sig_buf) override
	{
		++m_recv_count;
		//printf(">>>%s:%d:%d\n", m_consumer_name.c_str(), user_data, m_recv_count);
		jukey::util::Sleep(m_sleep_time);
	}

private:
	IComFactory* m_factory = nullptr;
	IAmqpClient* m_amqp_client = nullptr;
	std::string m_consumer_name;
	uint32_t m_sleep_time = 0;
	uint32_t m_recv_count = 0;
};

int main()
{
	IComFactory* factory = GetComFactory();
	if (!factory) {
		std::cout << "Get component factory failed!" << std::endl;
		return -1;
	}

	if (!factory->Init("./")) {
		std::cout << "Init component factory failed!" << std::endl;
		return -1;
	}

	AmqpProducer producer(factory);
	if (!producer.Init()) {
		std::cout << "Initialize producer failed!" << std::endl;
		return -1;
	}

	AmqpConsumer consumer1(factory, "consumer1", 1);
	if (!consumer1.Init()) {
		std::cout << "Initialize consumer1 failed!" << std::endl;
		return -1;
	}

	AmqpConsumer consumer2(factory, "consumer2", 20);
	if (!consumer2.Init()) {
		std::cout << "Initialize consumer2 failed!" << std::endl;
		return -1;
	}

	producer.PublishMsg();

	//// fanout
	//amqp_client->DeclareExchange("fanout-exchange", ExchangeType::FANOUT);
	//amqp_client->DeclareQueue("fanout-queue");
	//amqp_client->BindQueue("fanout-exchange", "fanout-queue", "");
	//amqp_client->Publish("fanout-exchange", "", "hello world");

	//// direct
	//amqp_client->DeclareExchange("direct-exchange", ExchangeType::DIRECT);
	//amqp_client->DeclareQueue("direct-queue1");
	//amqp_client->DeclareQueue("direct-queue2");
	//amqp_client->BindQueue("direct-exchange", "direct-queue1", "direct.routing1");
	//amqp_client->BindQueue("direct-exchange", "direct-queue2", "direct.routing2");
	//amqp_client->Publish("direct-exchange", "direct.routing1", "hello world queue1");
	//amqp_client->Publish("direct-exchange", "direct.routing2", "hello world queue2");

	//// topic
	//amqp_client->DeclareExchange("topic-exchange", ExchangeType::TOPIC);
	//amqp_client->DeclareQueue("topic-queue1");
	//amqp_client->DeclareQueue("topic-queue2");
	//amqp_client->BindQueue("topic-exchange", "topic-queue1", "topic.#");
	//amqp_client->BindQueue("topic-exchange", "topic-queue2", "topic.*");
	//amqp_client->Publish("topic-exchange", "topic.routing", "hello world 1");
	//amqp_client->Publish("topic-exchange", "topic.routing1", "hello world 2");
	//amqp_client->Publish("topic-exchange", "topic.routing.1", "hello world 3");

	while (true) {
		jukey::util::Sleep(10);  
	}

    // done
    return 0;
}