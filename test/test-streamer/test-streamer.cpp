// test-streamer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <Windows.h>

#include <iostream>

#include "com-factory.h"
#include "if-pipeline.h"
#include "if-element.h"
#include "if-pin.h"
#include "common-struct.h"
#include "if-media-engine.h"


using namespace jukey::base;
using namespace jukey::stmr;
using namespace jukey::com;

#define TEST_QUERY GetComFactory()->QueryInterface




#include <iostream>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

class MyMainThreadExecutor : public jukey::com::MainThreadExecutor 
{
public:
	MyMainThreadExecutor() : stopFlag(false) {
		// 启动主线程
		//workerThread = std::thread([this]() { this->run(); });
	}

	~MyMainThreadExecutor() {
		{
			std::lock_guard<std::mutex> lock(queueMutex);
			stopFlag = true;
		}
		cv.notify_all();
		//workerThread.join();
	}

	void RunInMainThread(std::function<void()> task) override {
		{
			std::lock_guard<std::mutex> lock(queueMutex);
			taskQueue.push(task);
		}
		cv.notify_all();
	}

	void Run() {
		while (true) {
			std::function<void()> task;
			{
				std::unique_lock<std::mutex> lock(queueMutex);
				cv.wait(lock, [this] { return !taskQueue.empty() || stopFlag; });

				if (stopFlag && taskQueue.empty())
					break;

				task = taskQueue.front();
				taskQueue.pop();
			}
			task();  // 执行任务
		}
	}

private:
	//std::thread workerThread;
	std::mutex queueMutex;
	std::condition_variable cv;
	std::queue<std::function<void()>> taskQueue;
	bool stopFlag;
};

class PipelineTest : jukey::sdk::IMediaEngineHandler
{
public:
	bool Init()
	{
		m_engine = CreateMediaEngine();

		if (jukey::com::ErrCode::ERR_CODE_OK != m_engine->Init(GetComFactory(), this, &m_executor)) {
			return false;
		}

		m_dev_mgr = m_engine->GetMediaDevMgr();

		m_pipeline = (IPipeline*)TEST_QUERY(CID_PIPELINE, IID_PIPELINE, "test");
		if (!m_pipeline) {
			std::cout << "Create pipeline failed!" << std::endl;
			return false;
		}

		if (ErrCode::ERR_CODE_OK != m_pipeline->Init("test")) {
			std::cout << "Init pipeline failed!" << std::endl;
			return false;
		}

		std::vector<jukey::sdk::CamDevice> devs = m_dev_mgr->GetCamDevList();
		for (auto& dev : devs) {
			std::cout << dev.cam_name << std::endl;
		}

		jukey::com::IProperty* prop1 = (jukey::com::IProperty*)GetComFactory()->QueryInterface(CID_PROPERTY,
			IID_PROPERTY, "test");
		prop1->SetU32Value("device-id", devs[0].cam_id);
		prop1->SetU32Value("resolution", 6);
		prop1->SetU32Value("pixel-format", 4);
		prop1->SetU32Value("frame-rate", 30);
		
		m_src_ele = m_pipeline->AddElement("cid-camera-element", prop1);
		if (!m_src_ele) {
			std::cout << "Create camera element failed!" << std::endl;
			return false;
		}

		m_convert_ele = m_pipeline->AddElement("cid-video-convert-element", nullptr);
		if (!m_convert_ele) {
			std::cout << "Create convert element failed!" << std::endl;
			return false;
		}

		jukey::com::IProperty* prop2 = (jukey::com::IProperty*)GetComFactory()->QueryInterface(CID_PROPERTY,
			IID_PROPERTY, "test");
		prop2->SetPtrValue("executor", &m_executor);

		m_sink_ele = m_pipeline->AddElement("cid-video-render-element", prop2);
		if (!m_sink_ele) {
			std::cout << "Create video render element failed!" << std::endl;
			return false;
		}

		m_convert_ele->SrcPins().front()->AddSinkPin(m_sink_ele->SinkPins().front());
		m_src_ele->SrcPins().front()->AddSinkPin(m_convert_ele->SinkPins().front());
		
		return true;
	} 

	void Start()
	{
		if (jukey::com::ErrCode::ERR_CODE_OK != m_pipeline->Start()) {
			return;
		}
		m_executor.Run();
	}

	virtual void OnAddMediaStream(const jukey::com::MediaStream& stream)
	{

	}

	//
	// Notify media stream removed
	//
	virtual void OnRemoveMediaStream(const jukey::com::MediaStream& stream)
	{

	}

	//
	// Audio stream test callback
	//
	virtual void OnAudioStreamEnergy(const jukey::com::Stream& stream,
		uint32_t energy)
	{

	}

	//
	// Play progress notify
	//
	virtual void OnPlayProgress(const jukey::com::MediaSrc& msrc, uint32_t prog)
	{

	}

	//
	// Run state
	//
	virtual void OnRunState(const std::string& desc)
	{

	}

	//
	// Audio stream statistics
	//
	virtual void OnAudioStreamStats(const jukey::com::MediaStream& stream,
		const jukey::com::AudioStreamStats& stats)
	{

	}

	//
	// Video stream statistics
	//
	virtual void OnVideoStreamStats(const jukey::com::MediaStream& stream,
		const jukey::com::VideoStreamStats& stats)
	{

	}

private:
	IPipeline* m_pipeline = nullptr;
	IElement* m_src_ele = nullptr;
	IElement* m_sink_ele = nullptr;
	IElement* m_convert_ele = nullptr;
	jukey::sdk::IMediaEngine* m_engine = nullptr;
	jukey::sdk::IMediaDevMgr* m_dev_mgr = nullptr;
	MyMainThreadExecutor m_executor;
};

int main()
{
	IComFactory* factory = GetComFactory();
	if (!factory) {
		std::cout << "Get component factory failed!" << std::endl;
		return false;
	}

	if (!factory->Init("../../../../../output/test/test-streamer/x64/Debug")) {
		std::cout << "Init component factory failed!" << std::endl;
		return false;
	}

	PipelineTest pt;
	if (!pt.Init()) {
		std::cout << "Init failed!" << std::endl;
		return -1;
	}
	 
	pt.Start(); 
	 
	return 0;
}

