#include "video_graphics_view.h"


namespace jukey::demo
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
QLabelWindow::QLabelWindow(QWidget* parent)
	: QWidget(parent), m_label(new QLabel("", parent))
{
	setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);
	m_label->setStyleSheet("background-color: rgba(0, 0, 0, 0); color: white; font-size: 16px;");
	m_label->setAlignment(Qt::AlignCenter);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(m_label);
	setLayout(layout);
	//setFixedSize(300, 100);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void QLabelWindow::UpdateLabelText(const std::string& text)
{
	m_label->setText(QString::fromStdString(text));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void QLabelWindow::UpdatePosition(const QPoint& point)
{
	move(point);
}

////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ParentEventFilter::ParentEventFilter(QLabelWindow* labelWindow,
	QWidget* parent)
	: QObject(parent), m_labelWindow(labelWindow), m_graphics_view(parent)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ParentEventFilter::eventFilter(QObject* obj, QEvent* event)
{
	if (event->type() == QEvent::Move) {
		QPoint globalPos = m_graphics_view->mapToGlobal(QPoint(10, 10));
		m_labelWindow->UpdatePosition(globalPos);
	}
	return QObject::eventFilter(obj, event);
}

////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
VideoGraphicsView::VideoGraphicsView(QWidget* parent)
	: QGraphicsView(parent)
	, m_scene(new QGraphicsScene(this))
	, m_labelWindow(new QLabelWindow(this))
{
	setScene(m_scene);
	setBackgroundBrush(Qt::black);

	// 初始化 QLabel window 位置
	UpdateLabelWindowPosition();
	m_labelWindow->show();

	// Install event filter on the parent window
	QWidget* main_wnd = (QWidget*)parent->parent()->parent();
	ParentEventFilter* filter = new ParentEventFilter(m_labelWindow, this);
	main_wnd->installEventFilter(filter);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
VideoGraphicsView::~VideoGraphicsView()
{
	delete m_labelWindow;
	delete m_scene;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoGraphicsView::UpdateLabelWindowPosition()
{
	// 更新 QLabel window 位置以跟随 QGraphicsView
	m_labelWindow->UpdatePosition(mapToGlobal(QPoint(10, 10)));
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void VideoGraphicsView::UpdateLabelText(const std::string& text)
{
	m_labelWindow->UpdateLabelText(text);
}

}