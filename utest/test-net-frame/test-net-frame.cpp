// test-net-frame.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

#include <iostream>
#include "gtest/gtest.h"
#include "common/util-net.h"
#include "lost-pkt-tracer.h"

using namespace jukey::com;
using namespace jukey::util;
using namespace jukey::net;


class TestSuit : public ::testing::Test
{
public:
  static void SetUpTestCase()
  {
    std::cout << __FUNCTION__ << std::endl;
  }

  static void TearDownTestCase()
  {
    std::cout << __FUNCTION__ << std::endl;
  }

  virtual void SetUp()
  {
    std::cout << __FUNCTION__ << std::endl;
  }

  virtual void TearDown()
  {
    std::cout << __FUNCTION__ << std::endl;
  }
};


TEST_F(TestSuit, testCase1)
{
  LostPktTracer tracer(5000000);

  tracer.AddLostPkt(2);
  tracer.AddLostPkt(3);
  tracer.AddLostPkt(8);
  tracer.AddLostPkt(10);
  tracer.AddLostPkt(11);
  tracer.AddLostPkt(17);
  tracer.AddLostPkt(18);
  tracer.AddLostPkt(19);

  PktLostInfo info = tracer.GetInfo();

  ASSERT_EQ(info.lost_count, 8);
  ASSERT_EQ(info.max_cts_lost_count, 3);
  ASSERT_EQ(info.avg_cts_lost_count, 2);
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
