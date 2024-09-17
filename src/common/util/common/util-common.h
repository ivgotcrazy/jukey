#pragma once

#include <string>

#include "common-enum.h"
#include "common-error.h"

namespace jukey::util
{

//
// Generate GUID for crossing platfrom
//
std::string GenerateGUID();

//
// Empty destruction for shared_ptr
//
void NoDestruct(uint8_t* p);

//
// Empty destruction for shared_ptr
//
void NoneDestructForVoid(void* p);

//
// Cross platform to get last error
//
int64_t GetError();

//
// Combine two uint32_t into uint64_t
//
uint64_t CombineTwoInt(uint32_t high, uint32_t low);

//
// Parse hight int
//
uint32_t FirstInt(uint64_t value);

//
// Parse lower int
//
uint32_t SecondInt(uint64_t value);

}
