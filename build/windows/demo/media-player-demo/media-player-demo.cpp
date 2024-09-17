#include "media-player-demo.h"
#include "log.h"

#include <QFileDialog>


#define EXECUTE_EVENT	(QEvent::Type)(QEvent::User + 100)

namespace jukey::demo
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MediaPlayerDemo::MediaPlayerDemo(QWidget* parent): QMainWindow(parent)
{
	ui.setupUi(this);

	setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);

	connect(ui.pushButton_open,
		&QPushButton::clicked,
		this,
		&MediaPlayerDemo::OnOpen);

	connect(ui.pushButton_start,
		&QPushButton::clicked,
		this,
		&MediaPlayerDemo::OnStart);

	connect(ui.pushButton_open,
		&QPushButton::clicked,
		this,
		&MediaPlayerDemo::OnPause);

	ui.horizontalSlider->setRange(0, 1000);

	connect(ui.horizontalSlider,
		&QSlider::sliderReleased,
		this,
		&MediaPlayerDemo::OnSliderReleased);

	connect(ui.horizontalSlider,
		&QSlider::sliderPressed,
		this,
		&MediaPlayerDemo::OnSliderPressed);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MediaPlayerDemo::~MediaPlayerDemo()
{}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool MediaPlayerDemo::event(QEvent* event)
{
	if (event->type() == EXECUTE_EVENT) {
		ExecuteEvent* e = (ExecuteEvent*)event;
		e->task();
		return true;
	}
	else {
		return QMainWindow::event(event);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaPlayerDemo::RunInMainThread(std::function<void()> task)
{
	ExecuteEvent* e = new ExecuteEvent(EXECUTE_EVENT);
	e->task = task;

	m_app->postEvent(this, e);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool MediaPlayerDemo::Init(QApplication* app)
{
	if (!app) {
		LOG_ERR("Invalid app");
		return false;
	}
	m_app = app;

	m_media_player = jukey::sdk::CreateMediaPlayer();
	if (!m_media_player) {
		LOG_ERR("Create media player failed");
		return false;
	}

	sdk::MediaPlayerParam param;
	param.com_path = "./";
	param.wnd      = (void*)ui.graphicsView->winId();
	param.handler  = this;
	param.executor = this;

	if (com::ERR_CODE_OK != m_media_player->Init(param)) {
		LOG_ERR("Init media player failed");
		return false;
	}

	QEvent::registerEventType(EXECUTE_EVENT);

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaPlayerDemo::OnOpenResult(bool result)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaPlayerDemo::OnPlayProgress(const com::MediaSrc& msrc, uint32_t prog)
{
	RunInMainThread([this, prog]() -> void {
		ui.horizontalSlider->setValue(prog);
	});
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaPlayerDemo::OnOpen()
{
	QString file_path = QFileDialog::getOpenFileName(nullptr, "选择文件", "", 
		"MP4 Files (*.mp4)");
	if (file_path.isEmpty()) {
		LOG_ERR("Invalid file path");
		return;
	}

	if (com::ERR_CODE_OK != m_media_player->Open(file_path.toStdString())) {
		LOG_ERR("Open media file failed");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaPlayerDemo::OnStart()
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaPlayerDemo::OnPause()
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaPlayerDemo::OnSliderPressed()
{
	m_media_player->SeekBegin();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaPlayerDemo::OnSliderReleased()
{
	m_media_player->SeekEnd(ui.horizontalSlider->value());
}

}