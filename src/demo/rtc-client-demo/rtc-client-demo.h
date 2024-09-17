#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_rtc-client-demo.h"
#include "sdk-mgr.h"
#include "demo-common.h"

namespace jukey::demo
{

//==============================================================================
// 
//==============================================================================
class RtcClientDemo : public QMainWindow, public ISdkEventHandler
{
	Q_OBJECT

public:
	RtcClientDemo(QWidget* parent = nullptr);
	~RtcClientDemo();

	bool Init(QCoreApplication* app);

	// ISdkEventHandler
	virtual void OnLoginResult(bool result) override;
	virtual void OnLogoutResult(bool result) override;
	virtual void OnJoinResult(bool result) override;
	virtual void OnLeaveResult(bool result) override;
  virtual void OnPubStream(const MediaStream& stream) override;
	virtual void OnUnpubStream(const MediaStream& stream) override;
	virtual void OnJoinGroupNotify(uint32_t user_id, uint32_t group_id) override;
	virtual void OnLeaveGroupNotify(uint32_t user_id, uint32_t group_id) override;
	virtual void OnRunState(const std::string& desc) override;
	virtual void OnStreamStats(const std::string& render_id,
		const std::string& stats) override;

public slots:
	void OnLogin();
	void OnJoinGroup();
	void OnOpenCamera();
	void OnOpenMicrophone();
	void OnOpenMediaFile();
	void OnSetting();

private:
	void UpdateOutputText(const std::string& str);
	void OpenCamera();
	void CloseCamera();
	void OpenMicrophone();
	void CloseMicrophone();
	void OpenMediaFile();
	void CloseMediaFile();

private:
	//============================
	//            INIT<---------|
	//             |            |
	//             | Login      |
	//             ↓     failed |
	//          LOGINING--------|
	//             |            |
	//             | success    |
	//             ↓     Logout |
	// |------->LOGINED---------|
	// |           |
	// |           | Join
	// | failed    ↓
	// |--------JOINING
	// |           |
	// |           | success
	// | Leave     ↓
	// |--------JOINED
	//============================
	enum class DemoState
	{
		INIT,
		LOGINING,
		LOGINED,
		LOGOUTING,
		JOINING,
		JOINED,
		LEAVING
	};

private:
	Ui::RtcClientDemoClass ui;
	SdkMgr m_sdk_mgr;
	std::string m_output_text;

	bool m_open_cam = false;
	bool m_open_mic = false;
	bool m_share_screen = false;
	bool m_share_media = false;

	std::string m_media_file;

	DemoState m_state = DemoState::INIT;
};

}
