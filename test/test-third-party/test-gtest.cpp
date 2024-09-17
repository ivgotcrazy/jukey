#include "test-gtest.h"

void GlobalEnviroment::SetUp()
{
	std::cout << "SetUp" << std::endl;
}

void GlobalEnviroment::TearDown()
{
	std::cout << "TearDown" << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

void MyTest::SetUp()
{
	std::cout << "SetUpTestCase" << std::endl;
}

void MyTest::TearDown()
{
	std::cout << "TearDownTestCase" << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

TEST_F(MyTest, test1)
{
	EXPECT_EQ(2, 2);
	EXPECT_EQ(3, 3);
	std::cout << "test1" << std::endl;
}

TEST_F(MyTest, test2)
{
	EXPECT_EQ(2, 2);
	EXPECT_EQ(3, 3);
	std::cout << "test2" << std::endl;
}

void DoGtestTest(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);
	testing::AddGlobalTestEnvironment(new GlobalEnviroment());
	RUN_ALL_TESTS();
}