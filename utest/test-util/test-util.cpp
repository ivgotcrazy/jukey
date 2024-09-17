// test-util.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "gtest/gtest.h"
#include "common/util-net.h"

using namespace jukey::com;
using namespace jukey::util;

TEST(ParseAddress, Normal1) 
{
    std::optional<Address> result = ParseAddress("TCP:127.0.0.1:8989");
    EXPECT_TRUE(result.has_value());
}

TEST(ParseAddress, Normal2)
{
    std::optional<Address> result = ParseAddress("TCP:www.test.com:8989");
    EXPECT_TRUE(result.has_value());
}

TEST(ParseAddress, NoProtocol)
{
    std::optional<Address> result = ParseAddress("127.0.0.1:8989");
    EXPECT_FALSE(result.has_value());
}

TEST(ParseAddress, NoPort)
{
    std::optional<Address> result = ParseAddress("TCP:127.0.0.1");
    EXPECT_FALSE(result.has_value());
}

TEST(ParseAddress, NoProtocolAndPort)
{
    std::optional<Address> result = ParseAddress("127.0.0.1");
    EXPECT_FALSE(result.has_value());
}

TEST(ParseAddress, InvalidPort)
{
    std::optional<Address> result = ParseAddress("TCP:127.0.0.1:89899");
    EXPECT_FALSE(result.has_value());
}

TEST(ParseAddress, InvalidProtocol)
{
    std::optional<Address> result = ParseAddress("KCP:127.0.0.1:8989");
    EXPECT_FALSE(result.has_value());
}

TEST(ParseAddress, InvalidIP)
{
    std::optional<Address> result = ParseAddress("TCP:127.0.0:89899");
    EXPECT_FALSE(result.has_value());
}

TEST(ParseAddress, InvalidDomain)
{
    std::optional<Address> result = ParseAddress("TCP:domain:89899");
    EXPECT_FALSE(result.has_value());
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

