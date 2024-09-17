// test-base-frame.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

#include "com-factory.h"
#include "if-unknown.h"
#include "if-com-test.h"

using namespace jukey::base;

int main()
{
	IComFactory* factory = GetComFactory();

#ifdef _WINDOWS
	if (!factory->Init("..\\..\\..\\..\\..\\output\\test\\test-component\\x64\\Debug")) {
		std::cout << "Init faild!" << std::endl;
		return -1;
	}
#else
	if (!factory->Init("./")) {
		std::cout << "Init faild!" << std::endl;
		return -1;
	}
#endif

	jukey::base::IUnknown* com = factory->CreateComponent("ComTest_CID", "test");
	if (nullptr == com) {
		std::cout << "Create component faild!" << std::endl;
		return -1;
	}

	test::IComTest* p = (test::IComTest*)com->QueryInterface("ComTest_IID");
	if (nullptr == p) {
		std::cout << "Query interface failed!" << std::endl;
		return -1;
	}

	std::cout << "Calc result: " << p->CalcResult() << std::endl;

	p->Release();

	return 0;
}
