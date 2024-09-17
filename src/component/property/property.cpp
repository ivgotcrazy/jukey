#include "property.h"
#include "common/util-common.h"

#include <sstream>

namespace jukey::com
{
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Property::Property(base::IComFactory* factory, const char* owner)
  : base::ProxyUnknown(nullptr)
  , base::ComObjTracer(factory, CID_PROPERTY, owner)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
Property::~Property()
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
base::IUnknown* Property::CreateInstance(base::IComFactory* factory, 
	const char* cid, const char* owner)
{
	if (strcmp(cid, CID_PROPERTY) == 0) {
		return new Property(factory, owner);
	}
	else {
		return nullptr;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void* Property::NDQueryInterface(const char* riid)
{
	if (0 == strcmp(riid, IID_PROPERTY)) {
		return static_cast<IProperty*>(this);
	}
	else {
		return ProxyUnknown::NDQueryInterface(riid);
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool Property::SetI32Value(CSTREF key, int32_t value)
{
	if (m_properties.find(key) != m_properties.end()) {
		return false;
	}

	PropItem item;
	item.prop_type = PropType::PROP_TYPE_INT32;
	item.prop_value = std::shared_ptr<int32_t>(new int32_t(value));

	m_properties.insert(std::make_pair(key, item));

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool Property::SetU32Value(CSTREF key, uint32_t value)
{
	if (m_properties.find(key) != m_properties.end()) {
		return false;
	}

	PropItem item;
	item.prop_type = PropType::PROP_TYPE_UINT32;
	item.prop_value = std::shared_ptr<uint32_t>(new uint32_t(value));

	m_properties.insert(std::make_pair(key, item));

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool Property::SetI64Value(CSTREF key, int64_t value)
{
	if (m_properties.find(key) != m_properties.end()) {
		return false;
	}

	PropItem item;
	item.prop_type = PropType::PROP_TYPE_INT64;
	item.prop_value = std::shared_ptr<int64_t>(new int64_t(value));

	m_properties.insert(std::make_pair(key, item));

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool Property::SetU64Value(CSTREF key, uint64_t value)
{
	if (m_properties.find(key) != m_properties.end()) {
		return false;
	}

	PropItem item;
	item.prop_type = PropType::PROP_TYPE_UINT64;
	item.prop_value = std::shared_ptr<uint64_t>(new uint64_t(value));

	m_properties.insert(std::make_pair(key, item));

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool Property::SetStrValue(CSTREF key, const char* value)
{
	if (m_properties.find(key) != m_properties.end()) {
		return false;
	}

	size_t len = strlen(value);
	std::shared_ptr<char> sv(new char[len + 1]);
	
#ifdef _WINDOWS
	strncpy_s(sv.get(), len + 1, value, len + 1);
#else
	strncpy(sv.get(), value, len + 1);
#endif

	PropItem item;
	item.prop_type = PropType::PROP_TYPE_STR;
	item.prop_value = sv;

	m_properties.insert(std::make_pair(key, item));

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool Property::SetPtrValue(CSTREF key, const void* value)
{
	if (m_properties.find(key) != m_properties.end()) {
		return false;
	}

	PropItem item;
	item.prop_type = PropType::PROP_TYPE_PTR;
	item.prop_value.reset((void*)value, util::NoneDestructForVoid);

	m_properties.insert(std::make_pair(key, item));

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
int32_t* Property::GetI32Value(CSTREF key)
{
	auto iter = m_properties.find(key);
	if (iter == m_properties.end()) {
		return nullptr;
	}

	if (iter->second.prop_type != PropType::PROP_TYPE_INT32) {
		return nullptr;
	}

	return (int32_t*)iter->second.prop_value.get();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t* Property::GetU32Value(CSTREF key)
{
	auto iter = m_properties.find(key);
	if (iter == m_properties.end()) {
		return nullptr;
	}

	if (iter->second.prop_type != PropType::PROP_TYPE_UINT32) {
		return nullptr;
	}

	return (uint32_t*)iter->second.prop_value.get();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
int64_t* Property::GetI64Value(CSTREF key)
{
	auto iter = m_properties.find(key);
	if (iter == m_properties.end()) {
		return nullptr;
	}

	if (iter->second.prop_type != PropType::PROP_TYPE_INT64) {
		return nullptr;
	}

	return (int64_t*)iter->second.prop_value.get();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint64_t* Property::GetU64Value(CSTREF key)
{
	auto iter = m_properties.find(key);
	if (iter == m_properties.end()) {
		return nullptr;
	}

	if (iter->second.prop_type != PropType::PROP_TYPE_UINT64) {
		return nullptr;
	}

	return (uint64_t*)iter->second.prop_value.get();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
const char* Property::GetStrValue(CSTREF key)
{
	auto iter = m_properties.find(key);
	if (iter == m_properties.end()) {
		return nullptr;
	}

	if (iter->second.prop_type != PropType::PROP_TYPE_STR) {
		return nullptr;
	}

	return (char*)iter->second.prop_value.get();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
const void* Property::GetPtrValue(CSTREF key)
{
	auto iter = m_properties.find(key);
	if (iter == m_properties.end()) {
		return nullptr;
	}

	if (iter->second.prop_type != PropType::PROP_TYPE_PTR) {
		return nullptr;
	}

	return (char*)iter->second.prop_value.get();
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string Property::Dump()
{
	std::stringstream ss;

	for (const auto& item : m_properties) {
		if (!ss.str().empty()) {
			ss << ", ";
		}
		switch (item.second.prop_type) {
			case PropType::PROP_TYPE_INT32:
			{
				int32_t* t = (int32_t*)item.second.prop_value.get();
				ss << item.first << ":" << *t;
				break;
			}
			case PropType::PROP_TYPE_UINT32:
			{
				uint32_t* t = (uint32_t*)item.second.prop_value.get();
				ss << item.first << ":" << *t;
				break;
			}
			case PropType::PROP_TYPE_INT64:
			{
				int64_t* t = (int64_t*)item.second.prop_value.get();
				ss << item.first << ":" << *t;
				break;
			}
			case PropType::PROP_TYPE_UINT64:
			{
				uint64_t* t = (uint64_t*)item.second.prop_value.get();
				ss << item.first << ":" << *t;
				break;
			}
			case PropType::PROP_TYPE_STR:
			{
				char* t = (char*)item.second.prop_value.get();
				ss << item.first << ":" << t;
				break;
			}
			case PropType::PROP_TYPE_PTR:
			{
				void* t = (void*)item.second.prop_value.get();
				ss << item.first << ":" << t;
			}
		}
	}

	return ss.str();
}

}
