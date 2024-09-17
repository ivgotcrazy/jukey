#pragma once

#include <map>
#include "opentelemetry/context/propagation/text_map_propagator.h"

using namespace opentelemetry;

namespace jukey::com
{

//==============================================================================
//
//==============================================================================
class MyCarrier : public context::propagation::TextMapCarrier
{
public:
  MyCarrier() = default;
  MyCarrier(const std::string& context);

  std::string ToString();

  // TextMapCarrier
  virtual nostd::string_view Get(nostd::string_view key) const noexcept override;
  virtual void Set(nostd::string_view key, nostd::string_view value) noexcept override;
  virtual bool Keys(nostd::function_ref<bool(nostd::string_view)> callback) const noexcept override;

private:
  std::map<nostd::string_view, nostd::string_view> m_key_values;
};

}