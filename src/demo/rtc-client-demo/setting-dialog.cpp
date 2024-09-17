#include "setting-dialog.h"
#include "setting-mgr.h"


namespace jukey::demo
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SettingDialog::SettingDialog(SdkMgr* mgr, QWidget* parent) : m_sdk_mgr(mgr)
{
	ui.setupUi(this);

	InitUI();

	connect(ui.comboBox_camera, &QComboBox::currentIndexChanged, this,
		&SettingDialog::OnCurrentCamChanged);

	connect(ui.comboBox_microphone, &QComboBox::currentIndexChanged, this,
		&SettingDialog::OnCurrentMicChanged);

	connect(ui.okButton, &QPushButton::clicked, this,
		&SettingDialog::OnConfirmButtonClicked);

	connect(ui.cancelButton, &QPushButton::clicked, this,
		&SettingDialog::OnCancelButtonClicked);

	connect(ui.pushButton_testMic, &QPushButton::clicked, this,
		&SettingDialog::OnTestMicClicked);

	connect(this, SIGNAL(audioEnergy(uint32_t)), this,
		SLOT(OnAudioEnergy(uint32_t)));

	connect(ui.tabWidget_setting, &QTabWidget::currentChanged, this,
		&SettingDialog::OnCurrentTabChanged);

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SettingDialog::~SettingDialog()
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SettingDialog::InitUI()
{
	// Load cameras
	std::vector<CamDevice> cam_list = m_sdk_mgr->GetCamDevices();
	for (const auto& cam : cam_list) {
		ui.comboBox_camera->addItem(
			QString::fromStdString(cam.cam_name).toUtf8(),
			quint32(cam.cam_id));
	}

	// Load microphones
	std::vector<MicDevice> mic_list = m_sdk_mgr->GetMicDevices();
	for (const auto& mic : mic_list) {
		ui.comboBox_microphone->addItem(
			QString::fromStdString(mic.mic_name).toUtf8(),
			quint32(mic.mic_id));
	}

	// Load speakers
	std::vector<SpkDevice> spk_list = m_sdk_mgr->GetSpkDevices();
	for (const auto& spk : spk_list) {
		ui.comboBox_speaker->addItem(
			QString::fromStdString(spk.spk_name).toUtf8(),
			quint32(spk.spk_id));
	}

	SetCurrentCam();
	SetCurrentMic();
	SetCurrentSpk();

	// Trigger to load attributes
	OnCurrentCamChanged();
	OnCurrentMicChanged();
	OnCurrentSpkChanged();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SettingDialog::SetCurrentMicAttributes()
{
	MicSetting setting = SettingMgr::Instance().Setting().mic_setting;

	for (auto i = 0; i < ui.comboBox_sampleRate->count(); i++) {
		if (ui.comboBox_sampleRate->itemData(i).toInt()
			== setting.sample_rate) {
			ui.comboBox_sampleRate->setCurrentIndex(i);
			break;
		}
	}

	for (auto i = 0; i < ui.comboBox_channelCount->count(); i++) {
		if (ui.comboBox_channelCount->itemData(i).toInt()
			== setting.sample_chnl) {
			ui.comboBox_channelCount->setCurrentIndex(i);
			break;
		}
	}

	for (auto i = 0; i < ui.comboBox_sampleBits->count(); i++) {
		if (ui.comboBox_sampleBits->itemData(i).toInt()
			== setting.sample_bits) {
			ui.comboBox_sampleBits->setCurrentIndex(i);
			break;
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SettingDialog::SetCurrentCam()
{
	CamSetting setting = SettingMgr::Instance().Setting().cam_setting;

	for (auto i = 0; i < ui.comboBox_camera->count(); i++) {
		if (ui.comboBox_camera->itemData(i) == setting.curr_cam) {
			ui.comboBox_camera->setCurrentIndex(i);
			break;
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SettingDialog::SetCurrentCamAttributes()
{
	CamSetting setting = SettingMgr::Instance().Setting().cam_setting;

	for (auto i = 0; i < ui.comboBox_resolution->count(); i++) {
		if (ui.comboBox_resolution->itemData(i) == setting.resolution) {
			ui.comboBox_resolution->setCurrentIndex(i);
			break;
		}
	}

	for (auto i = 0; i < ui.comboBox_pixelFormat->count(); i++) {
		if (ui.comboBox_pixelFormat->itemData(i) == setting.pixel_format) {
			ui.comboBox_pixelFormat->setCurrentIndex(i);
			break;
		}
	}

	ui.horizontalSlider_frameRate->setValue(setting.frame_rate);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SettingDialog::SetCurrentSpkAttributes()
{
	
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SettingDialog::SetCurrentSpk()
{
	SpkSetting setting = SettingMgr::Instance().Setting().spk_setting;

	for (auto i = 0; i < ui.comboBox_speaker->count(); i++) {
		if (ui.comboBox_speaker->itemData(i).toUInt() == setting.curr_spk) {
			ui.comboBox_speaker->setCurrentIndex(i);
			break;
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SettingDialog::SetCurrentMic()
{
	MicSetting setting = SettingMgr::Instance().Setting().mic_setting;

	for (auto i = 0; i < ui.comboBox_microphone->count(); i++) {
		if (ui.comboBox_microphone->itemData(i).toUInt() == setting.curr_mic) {
			ui.comboBox_microphone->setCurrentIndex(i);
			break;
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SettingDialog::OnConfirmButtonClicked()
{
	DemoSetting& setting = SettingMgr::Instance().Setting();

	setting.cam_setting.curr_cam =
		ui.comboBox_camera->currentData().toUInt();

	setting.cam_setting.resolution =
		ui.comboBox_resolution->currentData().toUInt();

	setting.cam_setting.pixel_format =
		ui.comboBox_pixelFormat->currentData().toUInt();

	setting.cam_setting.frame_rate =
		ui.horizontalSlider_frameRate->value();

	setting.mic_setting.curr_mic =
		ui.comboBox_microphone->currentData().toUInt();

	setting.mic_setting.sample_chnl =
		ui.comboBox_channelCount->currentData().toUInt();

	setting.mic_setting.sample_rate =
		ui.comboBox_sampleRate->currentData().toUInt();

	setting.mic_setting.sample_bits =
		ui.comboBox_sampleBits->currentData().toUInt();

	setting.spk_setting.curr_spk =
		ui.comboBox_speaker->currentData().toUInt();

	SettingMgr::Instance().UpdateSetting();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SettingDialog::OnCancelButtonClicked()
{
	// Do nothing
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SettingDialog::OnCurrentTabChanged()
{
	if (ui.tabWidget_setting->currentIndex() == 2 && !m_previewing) {
		CamParam param;
		param.frame_rate = 30;
		param.pixel_format = ui.comboBox_pixelFormat->currentData().toInt();
		param.resolution = ui.comboBox_resolution->currentData().toInt();

		void* wnd = (void*)ui.graphicsView_preview->winId();

		std::string dev_id = ui.comboBox_camera->currentData().toString().toStdString();

		m_sdk_mgr->StartPreviewCamera(dev_id, param, wnd);

		m_previewing = true;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SettingDialog::OnCurrentMicChanged()
{
	uint32_t dev_id = ui.comboBox_microphone->currentData().toUInt();

	MicDevice device;
	if (!m_sdk_mgr->GetMicDevice(dev_id, device)) {
		return;
	}

	// Load sample rate
	ui.comboBox_sampleRate->clear();
	for (const auto& item : device.srate_list) {
		ui.comboBox_sampleRate->addItem(QString::fromStdString(item.srate_name),
			quint32(item.srate_id));
	}

	// Load channel count
	ui.comboBox_channelCount->clear();
	for (const auto& item : device.chnls_list) {
		ui.comboBox_channelCount->addItem(QString::fromStdString(item.chnls_name),
			quint32(item.chnls_id));
	}

	// Load sample bits
	ui.comboBox_sampleBits->clear();
	for (const auto& item : device.sbits_list) {
		ui.comboBox_sampleBits->addItem(QString::fromStdString(item.sbits_name),
			quint32(item.sbits_id));
	}

	if (ui.comboBox_microphone->currentData().toUInt()
		== SettingMgr::Instance().Setting().mic_setting.curr_mic) {
		SetCurrentMicAttributes();
	}
}

//------------------------------------------------------------------------------
// resolution and pixel format items depend on camera
//------------------------------------------------------------------------------
void SettingDialog::OnCurrentCamChanged()
{
	uint32_t dev_id = ui.comboBox_camera->currentData().toUInt();

	CamDevice device;
	if (!m_sdk_mgr->GetCamDevice(dev_id, device)) {
		return;
	}

	ui.comboBox_resolution->clear();
	for (auto& resolution : device.res_list) {
		ui.comboBox_resolution->addItem(QString::fromStdString(resolution.res_name),
			quint32(resolution.res_id));
	}

	ui.comboBox_pixelFormat->clear();
	for (auto& format : device.format_list) {
		ui.comboBox_pixelFormat->addItem(QString::fromStdString(format.format_name),
			quint32(format.format_id));
	}

	/*ui.horizontalSlider_frameRate->setRange(device.min_fps, device.max_fps);
	ui.horizontalSlider_frameRate->setValue(15);
	ui.label_fps->setText("15 fps");*/

	ui.horizontalSlider_frameRate->setRange(1, 30);
	ui.horizontalSlider_frameRate->setValue(30);
	ui.label_fps->setText("30 fps");

	if (ui.comboBox_camera->currentData().toUInt()
		== SettingMgr::Instance().Setting().cam_setting.curr_cam) {
		SetCurrentCamAttributes();
	}
}

//------------------------------------------------------------------------------
// resolution and pixel format items depend on camera
//------------------------------------------------------------------------------
void SettingDialog::OnCurrentSpkChanged()
{
	uint32_t dev_id = ui.comboBox_speaker->currentData().toUInt();

	SpkDevice device;
	if (!m_sdk_mgr->GetSpkDevice(dev_id, device)) {
		return;
	}

	if (ui.comboBox_speaker->currentData().toUInt()
		== SettingMgr::Instance().Setting().spk_setting.curr_spk) {
		SetCurrentSpkAttributes();
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SettingDialog::customEvent(QEvent* event)
{
	if (event->type() == 9999) {
		//ui.progressBar_testMic
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SettingDialog::OnTestMicClicked()
{
	std::string dev_id = ui.comboBox_microphone->currentData().toString().toStdString();

	MicParam param;
	param.sample_chnl = ui.comboBox_channelCount->currentData().toInt();
	param.sample_bits = ui.comboBox_sampleBits->currentData().toInt();
	param.sample_rate = ui.comboBox_sampleRate->currentData().toInt();

	m_sdk_mgr->StartTestMic(dev_id, param, this);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SettingDialog::OnAudioEnergy(uint32_t energy)
{
	ui.progressBar_testMic->setValue(energy);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SettingDialog::OnAudioEnergy(const std::string& dev_id, uint32_t energy)
{
	emit audioEnergy(energy);
}

}