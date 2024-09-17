#include "element-factory.h"
#include "log.h"
#include "common/util-common.h"

using namespace jukey::com;

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ElementFactory::ElementFactory()
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ElementFactory& ElementFactory::Instance()
{
	static ElementFactory mgr;
	return mgr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
IElement* ElementFactory::CreateElement(CSTREF cid, IPipeline* pl, 
	com::IProperty* props)
{
	IElement* element = (IElement*)QI(cid.c_str(), IID_ELEMENT, "element factory");
	if (!element) {
		LOG_ERR("Create element:{} failed!", cid);
		return nullptr;
	}

	if (ERR_CODE_OK != element->Init(pl, props)) {
		LOG_ERR("Init element:{} failed!", element->Name());
		return nullptr;
	}

	return element;
}

//------------------------------------------------------------------------------
// Load all element entries
//------------------------------------------------------------------------------
void ElementFactory::Init(base::IComFactory* factory)
{
	if (m_inited) return;

	m_factory = factory;

	std::vector<std::string> element_cids;
	factory->GetComponents(IID_ELEMENT, element_cids);

	LOG_INF("Get component count:{}", element_cids.size());

	for (auto cid : element_cids) {
		IElement* element = (IElement*)QI(cid.c_str(), IID_ELEMENT, "element factory");
		if (!element) {
			LOG_ERR("QueryInterface by cid:{} failed!", cid.c_str());
			continue;
		}

		EleEntry entry;
		entry.cid = cid;
		entry.iid = IID_ELEMENT;
		entry.main_type = element->MainType();
		entry.sub_type = element->SubType();
		entry.media_type = element->MType();
		
		m_ele_entries.push_back(entry);

		element->Release();
	}

	m_inited = true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ElementFactory::GetElements(EleMediaType media_type, EleSubType sub_type,
	std::vector<IElement*>& elements)
{
	for (auto entry : m_ele_entries) {
		if (entry.sub_type != sub_type || entry.media_type != media_type) {
			continue;
		}

		IElement* element = (IElement*)QI(entry.cid.c_str(), entry.iid.c_str(), 
			"element factory");
		if (!element) {
			LOG_ERR("QueryInterface by cid:{} failed!", entry.cid);
			continue;
		}
		elements.push_back(element);
	}
}

}