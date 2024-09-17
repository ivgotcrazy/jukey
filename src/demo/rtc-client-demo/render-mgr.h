#pragma once

#include <vector>
#include <memory>
#include <string>
#include <mutex>

#include <QWidget>
#include <QtWidgets/QGraphicsView>

#include "common-enum.h"
#include "common-struct.h"


namespace jukey::demo
{

//==============================================================================
// 
//==============================================================================
class RenderMgr
{
public:
	static RenderMgr& Instance();

	void Init(QWidget* parent);

	void* AddRender(const std::string& render_id);

	void RemoveRender(const std::string& render_id);

	void* FindRender(const std::string& render_id);

	void UpdateLabelText(const std::string& render_id, const std::string& text);

private:
	struct RenderEntry
	{
		QGraphicsView* view = nullptr;
		std::string id;
	};
	typedef std::shared_ptr<RenderEntry> RenderEntrySP;

private:
	void UpdateRenderDisplay();
	void Update1x2Display();
	void UpdateNxNDisplay(uint32_t n);

private:
	std::vector<RenderEntrySP> m_renders;
	QWidget* m_parent_wnd = nullptr;
	std::mutex m_mutex;
};

}