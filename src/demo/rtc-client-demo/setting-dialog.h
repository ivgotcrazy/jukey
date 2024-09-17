#pragma once

#include <QDialog>
#include "ui_setting-dialog.h"
#include "sdk-mgr.h"

namespace jukey::demo
{

//==============================================================================
// 
//==============================================================================
class SettingDialog : public QDialog, public IAudioTestHandler
{
	Q_OBJECT
public:
	explicit SettingDialog(SdkMgr* mgr, QWidget* parent = 0);
	~SettingDialog();

	virtual void customEvent(QEvent* event) override;

	// IAudioTestHandler
	virtual void OnAudioEnergy(const std::string& dev_id, uint32_t energy) override;

signals:
	void audioEnergy(uint32_t energy);

public slots:
	void OnCurrentCamChanged();
	void OnCurrentMicChanged();
	void OnCurrentSpkChanged();

	void OnConfirmButtonClicked();
	void OnCancelButtonClicked();
	void OnTestMicClicked();
	void OnAudioEnergy(uint32_t energy);
	void OnCurrentTabChanged();

private:
	void InitUI();

	void SetCurrentMicAttributes();
	void SetCurrentCamAttributes();
	void SetCurrentSpkAttributes();

	void SetCurrentCam();
	void SetCurrentMic();
	void SetCurrentSpk();

private:
	Ui::Dialog ui;
	SdkMgr* m_sdk_mgr;
	bool m_previewing = false;
};

}