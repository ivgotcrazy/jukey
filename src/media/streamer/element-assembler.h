#pragma once

#include <string>
#include <vector>
#include <memory>

#include "if-pin.h"
#include "if-element.h"

#include "public/media-enum.h"

namespace jukey::stmr
{

//==============================================================================
//
//==============================================================================
struct LinkNode
{
	LinkNode(IElement* ele) : element(ele) {}

	LinkNode(IElement* ele, ISrcPin* src, ISinkPin* sink)
		: element(ele)
		, src_pin(src)
		, sink_pin(sink)
	{}

	IElement* element = nullptr;
	ISrcPin* src_pin = nullptr;
	ISinkPin* sink_pin = nullptr;
};
typedef std::shared_ptr<LinkNode> LinkNodeSP;
typedef std::vector<LinkNodeSP> LinkNodeVec;
typedef std::vector<LinkNodeVec> LinkNodeVV;
typedef std::vector<EleSubType> SubTypeVec;
typedef std::vector<IElement*> ElementVec;

//==============================================================================
//
//==============================================================================
class ElementAssembler
{
public:
	/**
	 * @brief Singleton
	 */
	static ElementAssembler& Instance();

	/**
	 * @brief Initialization
	 */
	void Init();

	com::ErrCode TryAutoLink(ISrcPin* src_pin, ISinkPin* sink_pin);

	bool DoLinkElementNodes(const LinkNodeVec& nodes);

	bool FindAvaliableLinks(ISrcPin* src_pin, ISinkPin* sink_pin,
		LinkNodeVV& result);

private:
	struct ElementNode;
	typedef std::shared_ptr<ElementNode> ElementNodeSP;
	typedef std::vector<ElementNodeSP> NodeVec;

	struct ElementNode
	{
		ElementNode(EleSubType type) : sub_type(type) {}

		EleSubType sub_type;
		NodeVec children;
	};
	typedef std::vector<NodeVec> NodePathVec;

private:
	void BuildLinkGraph();
	void TraverseLinkPath();
	void TraverseNodeTree(const ElementNodeSP& node, NodeVec& path, 
		NodePathVec& paths);
	std::vector<EleSubType> AssembleElements(EleSubType begin, EleSubType end);
	bool GetElementNodes(media::MediaType media_type, const SubTypeVec& sub_types, 
		LinkNodeVV& nodes);
	void TryLinkNodes(const LinkNodeVV& data, LinkNodeSP prev, int row, 
		LinkNodeVV& results);

private:
	bool m_inited = false;

	NodeVec m_root_nodes;

	NodePathVec m_link_paths;
};

}