#include <string>
#include "gtest/gtest.h"

// TEST(XXX, YYY)������XXX��TestSuit��YYY��TestCase
// TEST_F�е�Fָ����Fixture�����ּܣ�
// ����ͬһ��TestSuit�����ܻ���TEST/TEST_F��

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