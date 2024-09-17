#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPixmap>
#include <QVBoxLayout>
#include <QEvent>
#include <QMoveEvent>

namespace jukey::demo
{

//==============================================================================
// 
//==============================================================================
class QLabelWindow : public QWidget {
	Q_OBJECT

public:
	QLabelWindow(QWidget* parent = nullptr);

	void UpdateLabelText(const std::string& text);

	void UpdatePosition(const QPoint& point);

private:
	QLabel* m_label = nullptr;
};

//==============================================================================
// 
//==============================================================================
class ParentEventFilter : public QObject {
	Q_OBJECT

public:
	ParentEventFilter(QLabelWindow* labelWindow, QWidget* parent);

protected:
	bool eventFilter(QObject* obj, QEvent* event) override;

private:
	QWidget* m_graphics_view = nullptr;
	QLabelWindow* m_labelWindow = nullptr;
};

//==============================================================================
// 
//==============================================================================
class VideoGraphicsView : public QGraphicsView {
	Q_OBJECT

public:
	VideoGraphicsView(QWidget* parent = nullptr);
	~VideoGraphicsView();

	void UpdateLabelText(const std::string& text);

private:
	void UpdateLabelWindowPosition();

private:
	QGraphicsScene* m_scene = nullptr;
	QLabelWindow* m_labelWindow = nullptr;
};

}