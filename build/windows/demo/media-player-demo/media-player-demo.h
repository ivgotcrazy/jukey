#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_media-player-demo.h"
#include "if-media-player.h"
#include "common-struct.h"

namespace jukey::demo
{

typedef std::function<void(void*)> MainThreadTask;

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
class MediaPlayerDemo 
	: public QMainWindow
	, public com::MainThreadExecutor
	, public sdk::MediaPlayerHandler
{
	Q_OBJECT

public:
	MediaPlayerDemo(QWidget* parent = nullptr);
	~MediaPlayerDemo();

	bool Init(QApplication* app);

	// MainThreadExecutor
	virtual void RunInMainThread(std::function<void()> task) override;

	// QMainWindow
	virtual bool event(QEvent* event) override;

	// MediaPlayerHandler
	virtual void OnOpenResult(bool result) override;
	virtual void OnPlayProgress(const com::MediaSrc& msrc, uint32_t prog) override;

public slots:
	void OnOpen();
	void OnStart();
	void OnPause();
	void OnSliderPressed();
	void OnSliderReleased();

private:
	Ui::MediaPlayerDemoClass ui;

	sdk::IMediaPlayer* m_media_player = nullptr;

	QApplication* m_app = nullptr;

	bool m_started = false;
	bool m_paused = false;
};

}