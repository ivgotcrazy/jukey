#include "util-common.h"

#include <sstream>
#include <random>
#include <iomanip>
#include <chrono>
#include <thread>

#ifdef _WINDOWS
#include <windows.h>
#endif

namespace
{
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t RandomChar() {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, 255);
	return dis(gen);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string GenerateHex(const unsigned int len) {
	std::stringstream ss;
	for (unsigned int i = 0; i < len; i++) {
		const auto rc = RandomChar();
		std::stringstream hexstream;
		hexstream << std::setiosflags(std::ios::uppercase) << std::hex << rc;
		auto hex = hexstream.str();
		ss << (hex.length() < 2 ? '0' + hex : hex);
	}
	return ss.str();
}
}

namespace jukey::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
std::string GenerateGUID()
{
	return GenerateHex(4) + "-" + GenerateHex(2) + "-" + GenerateHex(2) 
		+ "-" + GenerateHex(4);
}

////------------------------------------------------------------------------------
//// 
////------------------------------------------------------------------------------
//bool operator!(const ErrCode& ec)
//{
//	return ec != ErrCode::ERR_CODE_OK;
//}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
//uint64_t Now()
//{
//	using namespace std::chrono;
//
//	time_point<system_clock, milliseconds> tp
//		= time_point_cast<milliseconds>(system_clock::now());
//
//	return (duration_cast<milliseconds>(tp.time_since_epoch())).count();
//}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
//uint64_t MicroNow()
//{
//	using namespace std::chrono;
//
//	time_point<system_clock, microseconds> tp
//		= time_point_cast<microseconds>(system_clock::now());
//
//	return (duration_cast<microseconds>(tp.time_since_epoch())).count();
//}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void NoDestruct(uint8_t* p) {}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void NoneDestructForVoid(void* p) {}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
//void Sleep(uint64_t ms)
//{
//	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
//}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
int64_t GetError()
{
#ifdef _WINDOWS
	return ::GetLastError();
#else
	return errno;
#endif
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint64_t CombineTwoInt(uint32_t high, uint32_t low)
{
	uint64_t result = high;
	result = result << 32;
	result |= low;

	return result;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t FirstInt(uint64_t value)
{
	return static_cast<uint32_t>(value >> 32);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t SecondInt(uint64_t value)
{
	return static_cast<uint32_t>(value & 0xFFFFFFFF);
}

}
