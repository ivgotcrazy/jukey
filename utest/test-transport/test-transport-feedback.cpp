#include "gtest/gtest.h"
#include "transport-common.h"
#include "common-struct.h"

using namespace jukey::com;
using namespace jukey::txp;

TEST(TransportFeedback, Normal)
{
	std::map<uint32_t, uint64_t> input_pkts;

	input_pkts.insert(std::make_pair(100, 10000));
	input_pkts.insert(std::make_pair(101, 10010));
	input_pkts.insert(std::make_pair(103, 10030));
	input_pkts.insert(std::make_pair(110, 10100));
	input_pkts.insert(std::make_pair(111, 10110));
	input_pkts.insert(std::make_pair(112, 10120));
	input_pkts.insert(std::make_pair(113, 10130));

	Buffer buf = BuildTransportFB(input_pkts);
	
	std::map<uint32_t, uint32_t> output_pkts = ParseTransportFB(buf);

	EXPECT_EQ(input_pkts.size(), output_pkts.size());

	auto input_iter = input_pkts.begin();
	auto output_iter = output_pkts.begin();
	for (uint32_t i = 0; i < input_pkts.size(); i++) {
		EXPECT_EQ(input_iter->first, output_iter->first);
		EXPECT_EQ(input_iter->second, output_iter->second);
		std::advance(input_iter, 1);
		std::advance(output_iter, 1);
	}	
}