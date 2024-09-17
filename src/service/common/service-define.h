#pragma once

namespace jukey::srv
{

//==============================================================================
//
//==============================================================================
#define PARSE_CONFIG_ENTRY_IMPL                                              \
template<typename T>                                                         \
bool ParseConfigEntry(const YAML::Node& root, std::vector<std::string> keys, \
  T& out)                                                                    \
{                                                                            \
  YAML::Node node = YAML::Clone(root);                                       \
  std::string key_str;                                                       \
                                                                             \
  for (auto& key : keys) {                                                   \
    if (key_str.empty()) {                                                   \
      key_str += key;                                                        \
    }                                                                        \
    else {                                                                   \
      key_str = key_str + ":" + key;                                         \
    }                                                                        \
    if (!node[key]) {                                                        \
      LOG_ERR("cannot find {} config", key_str);                             \
      return false;                                                          \
    }                                                                        \
    node = node[key];                                                        \
  }                                                                          \
                                                                             \
  out = node.as<T>();                                                        \
  if constexpr (std::is_same<T, std::string>::value) {                       \
    if (out.empty()) {                                                       \
      LOG_ERR("Invalid {}", key_str);                                        \
      return false;                                                          \
    }                                                                        \
  }                                                                          \
  else if constexpr (std::is_integral<T>::value) {                           \
    if (out == 0) {                                                          \
      LOG_ERR("Invalid {}", key_str);                                        \
      return false;                                                          \
    }                                                                        \
  }                                                                          \
  else {                                                                     \
    LOG_ERR("Unsupported type:{}", typeid(T));                               \
    return false;                                                            \
  }                                                                          \
                                                                             \
  LOG_INF("{} = {}", key_str, out);                                          \
                                                                             \
  return true;                                                               \
}                                                                            \
                                                                             \
template<>                                                                   \
bool ParseConfigEntry(const YAML::Node& root, std::vector<std::string> keys, \
  jukey::com::Address& out)                                                  \
{                                                                            \
  std::string addr_str;                                                      \
  if (!ParseConfigEntry(root, keys, addr_str)) {                             \
    return false;                                                            \
  }                                                                          \
                                                                             \
  std::optional<jukey::com::Address> addr                                    \
	  = jukey::util::ParseAddress(addr_str);                                   \
  if (addr.has_value()) {                                                    \
    out = addr.value();                                                      \
  }                                                                          \
  else {                                                                     \
    LOG_ERR("Invalid rabbitmq address!");                                    \
    return false;                                                            \
  }                                                                          \
                                                                             \
  return true;                                                               \
}

//==============================================================================
// 
//==============================================================================
#define ARRAY(...) {__VA_ARGS__}

//==============================================================================
//
//==============================================================================
#define PARSE_CONFIG(Node, Keys, Out)                               \
  if (!ParseConfigEntry(Node, std::vector<std::string>Keys, Out)) { \
    return false;                                                   \
  }

}