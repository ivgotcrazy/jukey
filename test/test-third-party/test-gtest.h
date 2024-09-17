#include <string>
#include "gtest/gtest.h"

// TEST(XXX, YYY)，其中XXX是TestSuit，YYY是TestCase
// TEST_F中的F指的是Fixture（脚手架）
// 对于同一个TestSuit，不能混用TEST/TEST_F宏

class GlobalEnviroment : public testing::Environment
{
public:
	virtual void SetUp();
	virtual void TearDown();
};

class MyTest : public testing::Test
{
protected:
	virtual void SetUp() override;
	virtual void TearDown() override;
};

void DoGtestTest(int argc, char** argv);