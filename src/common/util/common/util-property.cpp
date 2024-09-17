#include "util-property.h"

namespace jukey::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
IPropertyUP MakeProperty(base::IComFactory* factory, const std::string& owner)
{
	return IPropertyUP((com::IProperty*)factory->QueryInterface(CID_PROPERTY,
		IID_PROPERTY, owner));
}

}