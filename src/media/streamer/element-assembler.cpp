#include "element-assembler.h"
#include "element-factory.h"
#include "util-enum.h"
#include "common/util-string.h"
#include "common/util-common.h"
#include "cap-negotiate.h"
#include "log.h"

using namespace jukey::com;

namespace jukey::stmr
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
ElementAssembler& ElementAssembler::Instance()
{
	static ElementAssembler instance;
	return instance;
}

//------------------------------------------------------------------------------
// raw-proxy----------------|
//                          |
// capturer-----------------| |----------------->player
//                          ↓ |
// demuxer--->decoder--->converter--->encoder--->muxer
//               ↑           |
//             proxy         |--->mixer--->encoder--->recoder
//------------------------------------------------------------------------------
void ElementAssembler::BuildLinkGraph()
{
	ElementNodeSP capturer(new ElementNode(EleSubType::CAPTURER));
	ElementNodeSP player(new ElementNode(EleSubType::PLAYER));
	ElementNodeSP demuxer(new ElementNode(EleSubType::DEMUXER));
	ElementNodeSP decoder(new ElementNode(EleSubType::DECODER));
	ElementNodeSP converter(new ElementNode(EleSubType::CONVERTER));
	ElementNodeSP encoder(new ElementNode(EleSubType::ENCODER));
	ElementNodeSP muxer(new ElementNode(EleSubType::MUXER));
	ElementNodeSP proxy(new ElementNode(EleSubType::PROXY));

	capturer->children.push_back(converter);
	
	demuxer->children.push_back(decoder);
	decoder->children.push_back(converter);

	proxy->children.push_back(converter);
	
	converter->children.push_back(encoder);
	converter->children.push_back(player);

	encoder->children.push_back(muxer);

	m_root_nodes.push_back(capturer);
	m_root_nodes.push_back(demuxer);
	m_root_nodes.push_back(proxy);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ElementAssembler::TraverseLinkPath()
{
	NodeVec path;
	for (auto root : m_root_nodes) {
		TraverseNodeTree(root, path, m_link_paths);
	}

	if (m_link_paths.empty()) {
		LOG_ERR("Empty link path!");
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ElementAssembler::Init()
{
	if (m_inited) return;

	BuildLinkGraph();

	TraverseLinkPath();

	m_inited = true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ElementAssembler::TraverseNodeTree(const ElementNodeSP& node, NodeVec& path, 
	NodePathVec& paths)
{
	path.push_back(node);

	if (node->children.empty()) {
		paths.push_back(path);
	}
	else {
		for (auto item : node->children) {
			TraverseNodeTree(item, path, paths);
		}
	}

	path.pop_back();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::vector<EleSubType> ElementAssembler::AssembleElements(EleSubType begin, 
	EleSubType end)
{
	bool find_begin = false;
	bool find_end = false;

	std::vector<EleSubType> result;

	// Try to match link path
	for (auto path : m_link_paths) {
		for (auto node : path) {
			if (node->sub_type == begin) {
				find_begin = true;
				continue;
			}

			if (node->sub_type == end) {
				find_end = true;
				break;
			}

			if (find_begin) {
				result.push_back(node->sub_type);
			}
		}
		if (find_begin && find_end) {
			return result;
		}
		find_begin = false;
		find_end = false;
		result.clear();
	}
	
	return result; // Match no link path
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ElementAssembler::FindAvaliableLinks(ISrcPin* src_pin, ISinkPin* sink_pin,
	LinkNodeVV& result)
{
	// Find needed element subtypes
	std::vector<EleSubType> sub_types = AssembleElements(
		src_pin->Element()->SubType(), sink_pin->Element()->SubType());
	if (sub_types.empty()) {
		LOG_ERR("Empty auto link elements!");
		return false;
	}

	LOG_INF("Assemble elements result:");
	for (auto sub_type : sub_types) {
		LOG_INF("\t{}", media::util::ELE_SUB_TYPE_STR(sub_type));
	}

	LinkNodeVV node_vecs;
	if (!GetElementNodes(src_pin->MType(), sub_types, node_vecs)) {
		LOG_ERR("Get element nodes failed!");
		return false;
	}

	// Insert src pin at first
	LinkNodeVec begin_vec;
	LinkNodeSP begin(new LinkNode(src_pin->Element(), src_pin, nullptr));
	begin_vec.push_back(begin);
	node_vecs.insert(node_vecs.begin(), begin_vec);

	// Add sink pin at last
	LinkNodeVec end_vec;
	LinkNodeSP end(new LinkNode(sink_pin->Element(), nullptr, sink_pin));
	end_vec.push_back(end);
	node_vecs.push_back(end_vec);

	// Try link nodes by negotiating, to find all available links
	TryLinkNodes(node_vecs, nullptr, 0, result);

	return !result.empty();
}

//------------------------------------------------------------------------------
// TODO: 组件对象没有释放
// 根据subtype查找所有element
//------------------------------------------------------------------------------
bool ElementAssembler::GetElementNodes(media::MediaType media_type,
	const SubTypeVec& sub_types, LinkNodeVV& nodes)
{
	LOG_INF("Start get element nodes, media_type:{}", media_type);

	ElementFactory& factory = ElementFactory::Instance();

	for (auto sub_type : sub_types) {
		LOG_INF("=> sub_type:{}", (int)sub_type);

		ElementVec elements;
		// TODO: MediaType -> EleMediaType
		factory.GetElements((EleMediaType)media_type, sub_type, elements);
		if (elements.empty()) {
			LOG_ERR("Get elements by subtype:{} failed!", sub_type);
			return false;
		}

		LinkNodeVec node_vec;
		// TODO: one src pin and one sink pin?
		for (auto element : elements) {
			LOG_INF("Element:{}", element->Name());

			// TODO: Only exists one src pin and one sink pin
			if (element->SrcPins().empty() || element->SinkPins().empty()) {
				LOG_ERR("Invalid src or sink pin!");
				continue;
			}

			LinkNodeSP node(new LinkNode(element));
			node->src_pin = element->SrcPins().front();
			node->sink_pin = element->SinkPins().front();
			node_vec.push_back(node);
		}
		nodes.push_back(node_vec);
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void ElementAssembler::TryLinkNodes(const LinkNodeVV& data, LinkNodeSP prev,
	int row, LinkNodeVV& results)
{
	LOG_INF("Try link nodes, row:{}", row);

	static LinkNodeVec result;

	for (uint32_t i = 0; i < data[row].size(); i++) {
		LinkNodeSP curr = data[row][i];
		if (prev == nullptr) { // first row
			result.push_back(curr);
			TryLinkNodes(data, curr, row + 1, results);
			result.pop_back();
		}
		else {
			std::string cap = NegotiateCap(prev->src_pin->Caps(),
				curr->sink_pin->Caps());
			if (!cap.empty()) { // Negotiate success!!!
				result.push_back(curr);
				if ((size_t)row + 1 >= data.size()) { // last row
					results.push_back(result);
				}
				else {
					TryLinkNodes(data, curr, row + 1, results);
				}
				result.pop_back();
			}
			else {
				LOG_ERR("Negotiate failed:");
				LOG_ERR("\tSrc element:{}, caps:{}", prev->element->Name(),
					prev->src_pin->Caps());
				LOG_ERR("\tSink element:{} caps:{}", curr->element->Name(),
					curr->sink_pin->Caps());
			}
		}
	}
}


//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool ElementAssembler::DoLinkElementNodes(const LinkNodeVec& nodes)
{
	LinkNodeVec::const_reverse_iterator nxt = nodes.rend();
	LinkNodeVec::const_reverse_iterator cur = nodes.rend();

	for (cur = nodes.rbegin(); cur != nodes.rend(); ++cur) {
		if (nxt == nodes.rend()) {
			nxt = cur;
			continue;
		}

		ErrCode ec = cur->get()->src_pin->AddSinkPin(nxt->get()->sink_pin);
		if (ec != ERR_CODE_OK) {
			LOG_ERR("Element:{} link element:{} failed!",
				cur->get()->element->Name(),
				nxt->get()->element->Name());
			return false;
		}

		nxt = cur;
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
com::ErrCode ElementAssembler::TryAutoLink(ISrcPin* src_pin, ISinkPin* sink_pin)
{
	ElementAssembler& assembler = ElementAssembler::Instance();

	LinkNodeVV links;
	if (!assembler.FindAvaliableLinks(src_pin, sink_pin, links)) {
		LOG_ERR("Find avaliable links failed!");
		return ERR_CODE_FAILED;
	}

	LOG_INF("Find avaliable links:");
	for (auto link : links) {
		std::string link_str("\tlink: ");
		for (auto item : link) {
			link_str.append("->");
			link_str.append(item->element->Name());
		}
		LOG_INF("{}", link_str);
	}

	// Select one link to link
	bool flag = false;
	for (auto link : links) {
		if (DoLinkElementNodes(link)) {
			flag = true;
			break;
		}
	}

	if (!flag) {
		LOG_ERR("Link element nodes failed!");
		return ERR_CODE_FAILED;
	}

	// TODO: release components

	return ERR_CODE_OK;
}

}