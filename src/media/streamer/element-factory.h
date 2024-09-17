#pragma once

#include "if-element.h"
#include "com-factory.h"

namespace jukey::stmr
{

//==============================================================================
// 
//==============================================================================
class ElementFactory 
{
public:
	static ElementFactory& Instance();

	void Init(base::IComFactory* factory);

	IElement* CreateElement(CSTREF cid, IPipeline* pl, com::IProperty* props);

	void GetElements(EleMediaType media_type, EleSubType sub_type, 
		std::vector<IElement*>& elements);
	
private:
	ElementFactory();

	struct EleEntry
	{
		std::string  cid;
		std::string  iid;
		EleMainType  main_type;
		EleSubType   sub_type;
		EleMediaType media_type;
	};

private:
	bool m_inited = false;

	base::IComFactory* m_factory = nullptr;

	std::vector<EleEntry> m_ele_entries;
};

}