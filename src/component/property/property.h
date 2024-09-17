#pragma once

#include <map>
#include <memory>

#include "include/if-property.h"
#include "proxy-unknown.h"
#include "com-obj-tracer.h"

namespace jukey::com
{

//==============================================================================
// 
//==============================================================================
class Property 
	: public base::ProxyUnknown
	, public base::ComObjTracer
	, public IProperty
{
public:
	Property(base::IComFactory* factory, const char* owner);
	~Property();

	// ProxyUnknown
	COMPONENT_FUNCTION_DECL
	COMPONENT_IUNKNOWN_IMPL

	// IProperty
	virtual bool SetI32Value(CSTREF key, int32_t value) override;
	virtual bool SetU32Value(CSTREF key, uint32_t value) override;
	virtual bool SetI64Value(CSTREF key, int64_t value) override;
	virtual bool SetU64Value(CSTREF key, uint64_t value) override;
	virtual bool SetStrValue(CSTREF key, const char* value) override;
	virtual bool SetPtrValue(CSTREF key, const void* value) override;
	virtual int32_t* GetI32Value(CSTREF key) override;
	virtual uint32_t* GetU32Value(CSTREF key) override;
	virtual int64_t* GetI64Value(CSTREF key) override;
	virtual uint64_t* GetU64Value(CSTREF key) override;
	virtual const char* GetStrValue(CSTREF key) override;
	virtual const void* GetPtrValue(CSTREF key) override;
	virtual std::string Dump() override;

private:
	enum class PropType
	{
		PROP_TYPE_INVALID = 0,
		PROP_TYPE_INT32   = 1,
		PROP_TYPE_UINT32  = 2,
		PROP_TYPE_INT64   = 3,
		PROP_TYPE_UINT64  = 4,
		PROP_TYPE_STR     = 5,
		PROP_TYPE_PTR     = 6
	};

	typedef std::shared_ptr<void> PropValue;

	struct PropItem
	{
		PropType prop_type = PropType::PROP_TYPE_INVALID;
		PropValue prop_value;
	};

private:
	std::map<std::string, PropItem> m_properties;
};

}