#include <QMessageBox>
#include <QFileDialog>

#include "rtc-client-demo.h"
#include "setting-dialog.h"
#include "config-parser.h"
#include "setting-mgr.h"
#include "render-mgr.h"
#include "log.h"


namespace jukey::demo
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
RtcClientDemo::RtcClientDemo(QWidget* parent) : QMainWindow(parent)
{
	ui.setupUi(this);

	ui.pushButton_join->setDisabled(true);
	ui.pushButton_broadcastCam->setDisabled(true);
	ui.pushButton_broadcastMic->setDisabled(true);
	ui.pushButton_shareScreen->setDisabled(true);

	ui.widget_videoContainer->setStyleSheet("background-color:black");

	connect(ui.pushButton_login,
		&QPushButton::clicked,
		this,
		&RtcClientDemo::OnLogin);

	connect(ui.pushButton_join,
		&QPushButton::clicked,
		this,
		&RtcClientDemo::OnJoinGroup);

	connect(ui.pushButton_broadcastCam,
		&QPushButton::clicked,
		this,
		&RtcClientDemo::OnOpenCamera);

	connect(ui.pushButton_broadcastMic,
		&QPushButton::clicked,
		this,
		&RtcClientDemo::OnOpenMicrophone);

	connect(ui.pushButton_shareMedia,
		&QPushButton::clicked,
		this,
		&RtcClientDemo::OnOpenMediaFile);

	connect(ui.pushButton_setting,
		&QPushButton::clicked,
		this,
		&RtcClientDemo::OnSetting);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
RtcClientDemo::~RtcClientDemo()
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool RtcClientDemo::Init(QCoreApplication* app)
{
	if (!ConfigParser::Instance().Init("./rtc-client-demo.xml")) {
		QMessageBox::information(NULL, "Init", "Parse config failed!");
		return false;
	}

	if (!SettingMgr::Instance().Init("./rtc-client-demo.db")) {
		QMessageBox::information(NULL, "Init", "Load setting failed!");
		return false;
	}

	RenderMgr::Instance().Init(ui.widget_videoContainer);

	if (!m_sdk_mgr.Init(app, this)) {
		QMessageBox::information(NULL, "Init", "Initialize sdk manager failed!");
		return false;
	}

	ui.pushButton_login->setDisabled(false);
	ui.pushButton_join->setDisabled(true);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcClientDemo::OnLogin()
{
	if (m_state == DemoState::INIT) {
		uint32_t user_id = ui.lineEdit_user->text().toUInt();
		if (user_id == 0) {
			QMessageBox::information(NULL, "Login", "Invalid user ID!");
			return;
		}

		if (!m_sdk_mgr.Login(user_id)) {
			QMessageBox::information(NULL, "Login", "Login failed!");
			return;
		}

		m_state = DemoState::LOGINING;
		ui.pushButton_login->setText("登录...");

		ui.pushButton_login->setDisabled(true);
		ui.pushButton_join->setDisabled(true);
	}
	else if (m_state == DemoState::LOGINED) {
		if (!m_sdk_mgr.Logout()) {
			QMessageBox::information(NULL, "Login", "Login failed!");
		}

		m_state = DemoState::LOGOUTING;
		ui.pushButton_login->setText("登出...");

		ui.pushButton_login->setDisabled(true);
		ui.pushButton_join->setDisabled(true);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcClientDemo::OnJoinGroup()
{
	if (m_state == DemoState::LOGINED) {
		uint32_t group_id = ui.lineEdit_group->text().toUInt();
		if (group_id == 0) {
			QMessageBox::information(NULL, "Join", "Invalid group ID!");
			return;
		}

		if (!m_sdk_mgr.JoinGroup(group_id)) {
			QMessageBox::information(NULL, "Join", "Join group failed!");
			return;
		}

		m_state = DemoState::JOINING;
		ui.pushButton_join->setText("加入...");

		ui.pushButton_login->setDisabled(true);
		ui.pushButton_join->setDisabled(true);
	}
	else if (m_state == DemoState::JOINED) {
		if (!m_sdk_mgr.LeaveGroup()) {
			QMessageBox::information(NULL, "Join", "Leave group failed!");
			return;
		}

		m_state = DemoState::LEAVING;
		ui.pushButton_join->setText("离开...");

		ui.pushButton_login->setDisabled(true);
		ui.pushButton_join->setDisabled(true);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcClientDemo::OpenMicrophone()
{
	MicSetting setting = SettingMgr::Instance().Setting().mic_setting;

	MicParam param;
	param.dev_id = setting.curr_mic;
	param.sample_chnl = setting.sample_chnl;
	param.sample_bits = setting.sample_bits;
	param.sample_rate = setting.sample_rate;

	if (!m_sdk_mgr.OpenMicrophone(param)) {
		QMessageBox::information(NULL, "Camera", "Open microphone failed!");
		return;
	}

	UpdateOutputText("Open microphone success");

	ui.pushButton_broadcastMic->setText("停止广播麦克风");

	m_open_mic = true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcClientDemo::CloseMicrophone()
{
	MicSetting setting = SettingMgr::Instance().Setting().mic_setting;

	m_sdk_mgr.CloseMicrophone(setting.curr_mic);

	ui.pushButton_broadcastMic->setText("广播麦克风");

	m_open_mic = false;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcClientDemo::OpenCamera()
{
	CamSetting setting = SettingMgr::Instance().Setting().cam_setting;

	CamParam param;
	param.dev_id = setting.curr_cam;
	param.frame_rate = 30;
	param.pixel_format = setting.pixel_format;
	param.resolution = setting.resolution;

	if (!m_sdk_mgr.OpenCamera(param)) {
		QMessageBox::information(NULL, "Camera", "Open camera failed!");
		return;
	}

	UpdateOutputText("Open camera success");

	ui.pushButton_broadcastCam->setText("停止广播摄像头");

	m_open_cam = true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcClientDemo::CloseCamera()
{
	CamSetting setting = SettingMgr::Instance().Setting().cam_setting;

	m_sdk_mgr.CloseCamera(setting.curr_cam);

	ui.pushButton_broadcastCam->setText("广播摄像头");

	m_open_cam = false;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcClientDemo::OnSetting()
{
	SettingDialog* dialog = new SettingDialog(&m_sdk_mgr, this);
	if (dialog->exec() == QDialog::Accepted) { // confirm
		//TODO: detect change
	}

	delete dialog;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcClientDemo::OnOpenMicrophone()
{
	m_open_mic ? CloseMicrophone() : OpenMicrophone();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcClientDemo::OnOpenCamera()
{
	m_open_cam ? CloseCamera() : OpenCamera();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcClientDemo::OnOpenMediaFile()
{
	m_share_media ? CloseMediaFile() : OpenMediaFile();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcClientDemo::OpenMediaFile()
{
	QString file_path = QFileDialog::getOpenFileName(nullptr, "选择文件", "",
		"MP4 Files (*.mp4)");
	if (file_path.isEmpty()) {
		LOG_ERR("Invalid file path");
		return;
	}

	if (!m_sdk_mgr.OpenMediaFile(file_path.toStdString())) {
		LOG_ERR("Open media file failed!");
	}

	m_media_file = file_path.toStdString();
	m_share_media = true;

	ui.pushButton_shareMedia->setText("停止共享媒体");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcClientDemo::CloseMediaFile()
{
	m_sdk_mgr.CloseMediaFile(m_media_file);
	m_share_media = false;
	ui.pushButton_shareMedia->setText("共享媒体");
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcClientDemo::OnLoginResult(bool result)
{
	if (result) {
		UpdateOutputText("Login success");

		m_state = DemoState::LOGINED;
		ui.pushButton_login->setText("登出");

		ui.pushButton_login->setDisabled(false);
		ui.pushButton_join->setDisabled(false);
	}
	else {
		UpdateOutputText("Login failed");

		m_state = DemoState::INIT;
		ui.pushButton_login->setText("登录");

		ui.pushButton_login->setDisabled(false);
		ui.pushButton_join->setDisabled(true);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcClientDemo::OnLogoutResult(bool result)
{
	if (result) {
		UpdateOutputText("Logout success");
	}
	else {
		UpdateOutputText("Logout failed");
	}

	m_state = DemoState::INIT;
	ui.pushButton_login->setText("登录");

	ui.pushButton_login->setDisabled(false);
	ui.pushButton_join->setDisabled(true);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcClientDemo::OnJoinResult(bool result)
{
	if (result) {
		UpdateOutputText("Join group success");

		m_state = DemoState::JOINED;
		ui.pushButton_join->setText("离开");

		ui.pushButton_login->setDisabled(true);
		ui.pushButton_join->setDisabled(false);

		ui.pushButton_broadcastCam->setDisabled(false);
		ui.pushButton_broadcastMic->setDisabled(false);
	}
	else {
		UpdateOutputText("Join group failed");

		m_state = DemoState::LOGINED;
		ui.pushButton_join->setText("加入");

		ui.pushButton_login->setDisabled(false);
		ui.pushButton_join->setDisabled(false);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RtcClientDemo::OnLeaveResult(bool result)
{
	if (result) {
		UpdateOutputText("Leave group success");
	}
	else {
		UpdateOutputText("Leave group failed");
	}

	m_state = DemoState::LOGINED;
	ui.pushButton_join->setText("加入");

	ui.pushButton_login->setDisabled(false);
	ui.pushButton_join->setDisabled(false);
}

//------------------------------------------------------------------------------
//  
//------------------------------------------------------------------------------
void RtcClientDemo::OnPubStream(const MediaStream& stream)
{
	char out[128] = { 0 };
	if (stream.stream.stream_type == StreamType::AUDIO) {
		sprintf_s(out, 128, "User %d publish audio stream", 
			stream.src.user_id);
	}
	else if (stream.stream.stream_type == StreamType::VIDEO) {
		sprintf_s(out, 128, "User %d publish video stream", 
			stream.src.user_id);
	}
	else {
		sprintf_s(out, 128, "User %d publish invalid stream", 
			stream.src.user_id);
	}
	UpdateOutputText(out);
}

//------------------------------------------------------------------------------
//  
//------------------------------------------------------------------------------
void RtcClientDemo::OnUnpubStream(const MediaStream& stream)
{
	char out[128] = { 0 };
	if (stream.stream.stream_type == StreamType::AUDIO) {
		sprintf_s(out, 128, "User %d unpublish audio stream", 
			stream.src.user_id);
	}
	else if (stream.stream.stream_type == StreamType::VIDEO) {
		sprintf_s(out, 128, "User %d unpublish video stream", 
			stream.src.user_id);
	}
	else {
		sprintf_s(out, 128, "User %d unpublish invalid stream", 
			stream.src.user_id);
	}
	UpdateOutputText(out);
}

//------------------------------------------------------------------------------
//  
//------------------------------------------------------------------------------
void RtcClientDemo::OnJoinGroupNotify(uint32_t user_id, uint32_t group_id)
{
	char out[128] = { 0 };
	sprintf_s(out, 128, "User %d join group %d", user_id, group_id);

	UpdateOutputText(out);
}

//------------------------------------------------------------------------------
//  
//------------------------------------------------------------------------------
void RtcClientDemo::OnLeaveGroupNotify(uint32_t user_id, uint32_t group_id)
{
	char out[128] = { 0 };
	sprintf_s(out, 128, "User %d leave group %d", user_id, group_id);

	UpdateOutputText(out);
}

//------------------------------------------------------------------------------
//  
//------------------------------------------------------------------------------
void RtcClientDemo::OnRunState(const std::string& desc)
{
	UpdateOutputText(desc);
}

//------------------------------------------------------------------------------
//  
//------------------------------------------------------------------------------
void RtcClientDemo::UpdateOutputText(const std::string& str)
{
	QDateTime current_date_time = QDateTime::currentDateTime();
	QString cdt = current_date_time.toString("yyyy-MM-dd hh:mm:ss");

	if (m_output_text.empty()) {
		m_output_text.append(cdt.toStdString()).append("\n");
		m_output_text.append(str).append("\n");
	}
	else {
		m_output_text.append("\n").append(cdt.toStdString()).append("\n");
		m_output_text.append(str).append("\n");
	}

	ui.textEdit_info->setText(m_output_text.c_str());
}

//------------------------------------------------------------------------------
//  
//------------------------------------------------------------------------------
void RtcClientDemo::OnStreamStats(const std::string& render_id, 
	const std::string& stats)
{
	RenderMgr::Instance().UpdateLabelText(render_id, stats);
}

}