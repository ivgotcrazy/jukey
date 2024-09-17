#include "my-carrier.h"
#include "nlohmann/json.hpp"


using json = nlohmann::json;

namespace jukey::com
{

//------------------------------------------------------------------------------
// Json format context
//------------------------------------------------------------------------------
MyCarrier::MyCarrier(const std::string& context)
{
	json obj = json::parse(context);
	for (auto& item : obj.items()) {
		Set(item.key(), item.value());
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
nostd::string_view MyCarrier::Get(nostd::string_view key) const noexcept
{
	auto iter = m_key_values.find(key);
	if (iter != m_key_values.end()) {
		return iter->second;
	}
	else {
		return nostd::string_view();
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MyCarrier::Set(nostd::string_view key, nostd::string_view value) noexcept
{
	auto iter = m_key_values.find(key);
	if (iter != m_key_values.end()) {
		iter->second = value;
	}
	else {
		m_key_values.insert(std::pair(key, value));
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool MyCarrier::Keys(nostd::function_ref<bool(nostd::string_view)> callback) const noexcept
{
	for (auto& item : m_key_values) {
		if (!callback(item.first)) {
			return false;
		}
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string MyCarrier::ToString()
{
	json obj;

	for (auto& item : m_key_values) {
		obj[item.first.data()] = item.second.data();
	}

	return obj.dump();
}

}