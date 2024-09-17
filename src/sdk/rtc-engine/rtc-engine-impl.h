#pragma once

#include "if-rtc-engine.h"
#include "if-media-engine.h"
#include "if-session-mgr.h"
#include "protocol.h"
#include "async/session-async-proxy.h"
#include "thread/common-thread.h"
#include "rtc-common.h"
#include "protoc/stream.pb.h"
#include "msg-builder.h"

#include "login-processor.h"
#include "join-processor.h"
#include "camera-processor.h"
#include "microphone-processor.h"
#include "register-processor.h"
#include "stream-processor.h"
#include "media-file-processor.h"


namespace jukey::sdk
{

//==============================================================================
// 
//==============================================================================
class RtcEngineImpl 
	: public IRtcEngine
	, public IMediaEngineHandler
	, public util::CommonThread
{
public:
	static IRtcEngine* Instance();
	static void Release();

	// IRtcEngine
	virtual com::ErrCode Init(const RtcEngineParam& param) override;
	virtual com::ErrCode Login(uint32_t user_id) override;
	virtual com::ErrCode Logout() override;
	virtual com::ErrCode JoinGroup(uint32_t group_id) override;
	virtual com::ErrCode LeaveGroup() override;
	virtual com::ErrCode OpenCamera(const com::CamParam& param, 
		void* wnd) override;
	virtual com::ErrCode CloseCamera(uint32_t dev_id) override;
	virtual com::ErrCode OpenMic(const com::MicParam& param) override;
	virtual com::ErrCode CloseMic(uint32_t dev_id) override;
	virtual com::ErrCode OpenMediaFile(const std::string& file, void* wnd) override;
	virtual com::ErrCode CloseMediaFile(const std::string& file) override;
	virtual com::ErrCode StartRecvAudio(const com::MediaStream& stream) override;
	virtual com::ErrCode StopRecvAudio(const com::MediaStream& stream) override;
	virtual com::ErrCode StartRecvVideo(const com::MediaStream& stream, 
		void* wnd) override;
	virtual com::ErrCode StopRecvVideo(const com::MediaStream& stream, 
		void* wnd) override;
	virtual com::ErrCode StartMicTest(const std::string& dev_id) override;
	virtual com::ErrCode StopMicTest(const std::string& dev_id) override;
	virtual com::ErrCode StartRecord(const std::string& ouput_file) override;
	virtual com::ErrCode StopRecord() override;
	virtual IMediaDevMgr* GetMediaDevMgr() override;

	// IMediaEngineHandler
	virtual void OnAddMediaStream(const com::MediaStream& stream) override;
	virtual void OnRemoveMediaStream(const com::MediaStream& stream) override;
	virtual void OnAudioStreamEnergy(const com::Stream& stream,
		uint32_t energy) override;
	virtual void OnPlayProgress(const com::MediaSrc& msrc, 
		uint32_t prog) override;
	virtual void OnRunState(const std::string& desc) override;
	virtual void OnAudioStreamStats(const com::MediaStream& stream,
		const com::AudioStreamStats& stats) override;
	virtual void OnVideoStreamStats(const com::MediaStream& stream,
		const com::VideoStreamStats& stats) override;

	// CommonThread
	virtual void OnThreadMsg(const com::CommonMsg& msg) override;

private:
	com::ErrCode CheckParam(const RtcEngineParam& param);
	com::ErrCode InitSessionMgr();

	void OnSessionData(const com::CommonMsg& msg);
	void OnSessionClosed(const com::CommonMsg& msg);
	void OnSessionCreateResult(const com::CommonMsg& msg);

	void OnPubStreamRsp(const com::MediaStream& stream, const com::Buffer& buf);
	void OnPubStreamTimeout(const com::MediaStream& stream);
	void OnPubStreamError(const com::MediaStream& stream, com::ErrCode ec);

	void OnUnpubStreamRsp(const com::MediaStream& stream, const com::Buffer& buf);
	void OnUnpubStreamTimeout(const com::MediaStream& stream);
	void OnUnpubStreamError(const com::MediaStream& stream, com::ErrCode ec);

	void OnPubMediaRsp(const com::MediaStream& stream, const com::Buffer& buf);
	void OnPubMediaTimeout(const com::MediaStream& stream);
	void OnPubMediaError(const com::MediaStream& stream, com::ErrCode ec);

	void OnUnpubMediaRsp(const com::MediaStream& stream, const com::Buffer& buf);
	void OnUnpubMediaTimeout(const com::MediaStream& stream);
	void OnUnpubMediaError(const com::MediaStream& stream, com::ErrCode ec);

	void OnJoinGroupNotify(const com::Buffer& buf);
	void OnPublishMediaNotify(const com::Buffer& buf);
	void OnUnpublishMediaNotify(const com::Buffer& buf);

	void OnLoginSendChannelNotify(const com::Buffer& buf);
	void SendLoginSendChannelAck(const com::Buffer& buf,
		const prot::LoginSendChannelNotify& notify);
	void StartSendStream(const prot::LoginSendChannelNotify& notify);
	void OnUserLeaveGroupNotify(const com::Buffer& buf);

	void PublishStream(const com::MediaStream& stream);
	void UnpublishStream(const com::MediaStream& stream);

	com::ErrCode PublishGroupStream(const com::MediaStream& stream);
	com::ErrCode UnpublishGroupStream(const com::MediaStream& stream);

	void SendPublishMediaReq(const com::MediaStream& stream);
	void SendUnpublishMediaReq(const com::MediaStream& stream);

private:
	friend class LoginProcessor;
	friend class JoinProcessor;
	friend class CameraProcessor;
	friend class MicrophoneProcessor;
	friend class RegisterProcessor;
	friend class StreamProcessor;
	friend class MediaFileProcessor;

	LoginProcessor m_login_processor;
	JoinProcessor m_join_processor;
	CameraProcessor m_camera_processor;
	MicrophoneProcessor m_microphone_processor;
	RegisterProcessor m_register_processor;
	StreamProcessor m_stream_processor;
	MediaFileProcessor m_media_file_processor;

private:
	RtcEngineImpl();
	static RtcEngineImpl* s_rtc_engine;

	base::IComFactory* m_factory = nullptr;
	IMediaEngine* m_media_engine = nullptr;

	RtcEngineParam m_engine_param;
	RtcEngineData m_engine_data;
	RtcEngineState m_engine_state = RtcEngineState::INVALID;

	std::recursive_mutex m_mutex;

	// Connect to proxy service
	com::Address m_proxy_addr;
	net::SessionId m_proxy_session = INVALID_SESSION_ID;

	// Initialize synchronously
	std::promise<bool> m_init_promise;

	// Helper
	util::SessionAsyncProxySP m_async_proxy;
	MsgBuilderUP m_msg_builder;

	uint32_t m_cur_seq = 0;
};

}