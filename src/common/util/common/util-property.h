#pragma once

#include <memory>

#include "com-factory.h"
#include "if-property.h"

namespace jukey::util
{

//==============================================================================
// 
//==============================================================================
struct PropertyDeleter
{
	void operator()(com::IProperty* prop)
	{
		prop->Release();
	}
};
typedef std::unique_ptr<com::IProperty, PropertyDeleter> IPropertyUP;

// For auto manage of property
IPropertyUP MakeProperty(base::IComFactory* factory, const std::string& owner);

}