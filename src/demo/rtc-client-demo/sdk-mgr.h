#pragma once

#include <iostream>
#include <functional>

#include <QGuiApplication>
#include <QDir>

#include "if-rtc-engine.h"
#include "common-enum.h"
#include "common-struct.h"

using namespace jukey::sdk;
using namespace jukey::com;

namespace jukey::demo
{

typedef std::function<void(const MediaStream&, void*)> MainThreadTask;

//==============================================================================
// 
//==============================================================================
class ExecuteEvent : public QEvent
{
public:
	ExecuteEvent(QEvent::Type type) : QEvent(type) {}

	std::function<void()> task;
};

//==============================================================================
// 
//==============================================================================
class LoginResultEvent : public QEvent
{
public:
	LoginResultEvent(QEvent::Type type, bool r) : QEvent(type), result(r) {}
	bool result;
};

//==============================================================================
// 
//==============================================================================
class JoinResultEvent : public QEvent
{
public:
	JoinResultEvent(QEvent::Type type, bool r) : QEvent(type), result(r) {}
	bool result;
};

//==============================================================================
// 
//==============================================================================
class JoinGroupNotifyEvent : public QEvent
{
public:
	JoinGroupNotifyEvent(QEvent::Type type, uint32_t user) 
		: QEvent(type), user_id(user) {}

	uint32_t user_id = 0;
};

//==============================================================================
// 
//==============================================================================
class LeaveGroupNotifyEvent : public QEvent
{
public:
	LeaveGroupNotifyEvent(QEvent::Type type, uint32_t user)
		: QEvent(type), user_id(user) {}

	uint32_t user_id = 0;
};

//==============================================================================
// 
//==============================================================================
class PublishEvent : public QEvent
{
public:
	PublishEvent(QEvent::Type type, MediaStream* si) 
    : QEvent(type), stream_info(si) {}
	MediaStream* stream_info = nullptr;
};

//==============================================================================
// 
//==============================================================================
class UnpublishEvent : public QEvent
{
public:
	UnpublishEvent(QEvent::Type type, MediaStream* si)
		: QEvent(type), stream_info(si) {}
	MediaStream* stream_info = nullptr;
};

//==============================================================================
// 
//==============================================================================
class ISdkEventHandler
{
public:
	virtual void OnLoginResult(bool result) = 0;
	virtual void OnLogoutResult(bool result) = 0;
	virtual void OnJoinResult(bool result) = 0;
	virtual void OnLeaveResult(bool result) = 0;
  virtual void OnPubStream(const MediaStream& stream) = 0;
	virtual void OnUnpubStream(const MediaStream& stream) = 0;
	virtual void OnJoinGroupNotify(uint32_t user_id, uint32_t group_id) = 0;
	virtual void OnLeaveGroupNotify(uint32_t user_id, uint32_t group_id) = 0;
	virtual void OnRunState(const std::string& desc) = 0;
	virtual void OnStreamStats(const std::string& render_id, 
		const std::string& stats) = 0;
};

//==============================================================================
// 
//==============================================================================
class IAudioTestHandler
{
public:
	virtual void OnAudioEnergy(const std::string& dev_id, uint32_t energy) = 0;
};

//==============================================================================
// 
//==============================================================================
class SdkMgr: public QObject
	, public jukey::sdk::IRtcEngineHandler
	, public jukey::com::MainThreadExecutor
	, public jukey::sdk::IDevEventHandler
{
public:
	SdkMgr();

	bool Init(QCoreApplication* app, ISdkEventHandler* handler);
	bool Login(uint32_t user_id);
	bool Logout();
	bool JoinGroup(uint32_t group_id);
	bool LeaveGroup();

	std::vector<CamDevice> GetCamDevices();
	std::vector<MicDevice> GetMicDevices();
	std::vector<SpkDevice> GetSpkDevices();
	bool GetCamDevice(uint32_t dev_id, CamDevice& dev);
	bool GetMicDevice(uint32_t dev_id, MicDevice& dev);
	bool GetSpkDevice(uint32_t dev_id, SpkDevice& dev);

	bool OpenCamera(const CamParam& param);
	void CloseCamera(uint32_t dev_id);

	bool OpenMicrophone(const MicParam& param);
	void CloseMicrophone(uint32_t dev_id);

	bool OpenMediaFile(const std::string& file);
	void CloseMediaFile(const std::string& file);

	bool StartRecvAudio(const MediaStream& stream);
	bool StopRecvAudio(const MediaStream& stream);

	bool StartRecvVideo(const MediaStream& stream, void* wnd);
	bool StopRecvVideo(const MediaStream& stream, void* wnd);

	bool StartTestMic(const std::string& dev_id,
		const MicParam& param, 
		IAudioTestHandler* handler);
	void StopTestMic(const std::string& dev_id);

	void StartPreviewCamera(const std::string& dev_id,
		const CamParam& param, 
		void* wnd);
	void StopPreviewCamera(const std::string& dev_id, void* wnd);

	// QObject
	virtual bool event(QEvent* event) override;

	// IDevEventHandler
	virtual void OnAddCamera(const CamDevice& device) override {}
	virtual void OnDelCamera(const CamDevice& device) override {}
	virtual void OnAddMicrophone(const MicDevice& device) override {}
	virtual void OnDelMicrophone(const MicDevice& device) override {}

	// IRtcEngineHandler
	virtual void OnLoginResult(uint32_t user_id, 
		ErrCode result) override;
	virtual void OnLogoutResult(uint32_t user_id, 
		ErrCode result) override;
	virtual void OnJoinGroupResult(uint32_t group_id, 
		ErrCode result) override;
	virtual void OnLeaveGroupResult(uint32_t group_id, 
		ErrCode result) override;
	virtual void OnSubStreamResult(const MediaStream& stream,
		ErrCode result) override;
	virtual void OnPubStreamNotify(uint32_t group_id, 
		const MediaStream& stream,
		bool operation) override;
	virtual void OnAudioDevEnergy(const std::string& dev_id, 
		uint32_t energy) override;
	virtual void OnJoinGroupNotify(uint32_t user_id,
		uint32_t group_id) override;
	virtual void OnLeaveGroupNotify(uint32_t user_id,
		uint32_t group_id) override;
	virtual void OnRunState(const std::string& desc) override;
	virtual void OnAudioStreamStats(const com::MediaStream& stream,
		const com::AudioStreamStats& stats) override;
	virtual void OnVideoStreamStats(const com::MediaStream& stream,
		const com::VideoStreamStats& stats) override;

	// MainThreadExecutor
	virtual void RunInMainThread(std::function<void()> task) override;

private:
	void OnPubAudioNotify(uint32_t group_id, const MediaStream& stream);
	void OnUnpubAudioNotify(uint32_t group_id, const MediaStream& stream);

	void OnPubVideoNotify(uint32_t group_id, const MediaStream& stream);
	void OnUnpubVideoNotify(uint32_t group_id, const MediaStream& stream);

	void DoStartRecvAudio(const MediaStream& stream);
	void DoStopRecvAudio(const MediaStream& stream);

	void DoStartRecvVideo(const MediaStream& stream);
	void DoStopRecvVideo(const MediaStream& stream);

private:
	IRtcEngine* m_rtc_engine = nullptr;
	IMediaDevMgr* m_dev_mgr = nullptr;

	QCoreApplication* m_app = nullptr;

	MediaStream m_stream;

	ISdkEventHandler* m_event_handler = nullptr;

	std::string m_test_mic;
	IAudioTestHandler* m_test_handler = nullptr;

	void* m_preview_wnd = nullptr;

	std::map<std::string, com::MediaStream> m_streams;
};

}