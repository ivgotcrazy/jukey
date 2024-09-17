#include "render-mgr.h"

#include "qlabel.h"
#include "video_graphics_view.h"

namespace jukey::demo
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
RenderMgr& RenderMgr::Instance()
{
	static RenderMgr mgr;
	return mgr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RenderMgr::Init(QWidget* parent)
{
	m_parent_wnd = parent;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RenderMgr::Update1x2Display()
{
	QSize parent_size = m_parent_wnd->size();

	QGraphicsView* left_view = m_renders[0]->view;
	QGraphicsView* right_view = m_renders[1]->view;

	left_view->setGeometry(0, parent_size.height() / 4,
		parent_size.width() / 2, parent_size.height() / 2);

	right_view->setGeometry(parent_size.width() / 2, parent_size.height() / 4,
		parent_size.width() / 2, parent_size.height() / 2);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RenderMgr::UpdateNxNDisplay(uint32_t n)
{
	QSize parent_size = m_parent_wnd->size();

	for (auto index = 0; index < m_renders.size(); index++) {
		m_renders[index]->view->setGeometry(
			(index % n) * parent_size.width() / n,
			(index / n) * parent_size.height() / n,
			parent_size.width() / n,
			parent_size.height() / n);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RenderMgr::UpdateRenderDisplay()
{
	uint32_t render_count = m_renders.size();

	if (render_count == 0) return;

	if (render_count == 1) {
		UpdateNxNDisplay(1);
	}
	else if (render_count == 2) {
		Update1x2Display();
	}
	else if (render_count <= 4) {
		UpdateNxNDisplay(2);
	}
	else if (render_count <= 9) {
		UpdateNxNDisplay(3);
	}
	else {
		// Do nothing
	}

	for (auto& item : m_renders) {
		item->view->show();
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* RenderMgr::AddRender(const std::string& render_id)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_renders.size() >= 9) {
		return nullptr;
	}

	RenderEntrySP renderer(new RenderEntry());
	renderer->id = render_id;
	//renderer->view = new QGraphicsView(m_parent_wnd);
	renderer->view = new VideoGraphicsView(m_parent_wnd);

	m_renders.push_back(renderer);

	UpdateRenderDisplay();

	return (void*)renderer->view->winId();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RenderMgr::RemoveRender(const std::string& render_id)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto it = m_renders.begin(); it != m_renders.end(); it++) {
		if ((*it)->id == render_id) {
			delete (*it)->view;
			m_renders.erase(it);
			UpdateRenderDisplay();
			return;
		}
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* RenderMgr::FindRender(const std::string& render_id)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto it = m_renders.begin(); it != m_renders.end(); it++) {
		if ((*it)->id == render_id) {
			return (void*)((*it)->view->winId());
		}
	}

	return nullptr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void RenderMgr::UpdateLabelText(const std::string& render_id, 
	const std::string& text)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (auto& item : m_renders) {
		if (item->id == render_id) {
			VideoGraphicsView* view = dynamic_cast<VideoGraphicsView*>(item->view);
			if (view) {
				view->UpdateLabelText(text);
			}
			break;
		}
	}
}

}