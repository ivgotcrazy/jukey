#include "test-gtest.h"
#include "test-spdlog.h"


int main(int argc, char** argv)
{
	DoGtestTest(argc, argv);

	DoSpdlogTest();

	system("PAUSE");

	return 0;
}