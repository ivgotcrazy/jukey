#pragma once

#include <string>

#include "common-define.h"
#include "component.h"

namespace jukey::com
{

#define CID_PROPERTY "cid-property"
#define IID_PROPERTY "iid-property"

//==============================================================================
// 
//==============================================================================
class IProperty : public base::IUnknown
{
public:
	virtual bool SetI32Value(CSTREF key, int32_t value) = 0;
	virtual bool SetU32Value(CSTREF key, uint32_t value) = 0;
	virtual bool SetI64Value(CSTREF key, int64_t value) = 0;
	virtual bool SetU64Value(CSTREF key, uint64_t value) = 0;
	virtual bool SetStrValue(CSTREF key, const char* value) = 0;
	virtual bool SetPtrValue(CSTREF key, const void* value) = 0;

	virtual int32_t* GetI32Value(CSTREF key) = 0;
	virtual uint32_t* GetU32Value(CSTREF key) = 0;
	virtual int64_t* GetI64Value(CSTREF key) = 0;
	virtual uint64_t* GetU64Value(CSTREF key) = 0;
	virtual const char* GetStrValue(CSTREF key) = 0;
	virtual const void* GetPtrValue(CSTREF key) = 0;

	virtual std::string Dump() = 0;
};

}