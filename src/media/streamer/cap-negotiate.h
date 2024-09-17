#pragma once

#include <string>
#include <vector>

#include "common-define.h"

namespace jukey::stmr
{

/**
 * @brief Take negotiation between src pin and sink pins
 * @param src capabilities of src pin
 * @param sinks capabilities of sink pins
 */
std::string NegotiateCap(CSTREF src, const std::vector<std::string>& sinks);


/**
 * @brief Take negotiation between src pin and sink pin
 * @param src capabilities of src pin
 * @param sink capabilities of sink pin
 * @return negotiated capabilities
 */
std::string NegotiateCap(CSTREF src, CSTREF sink);

}