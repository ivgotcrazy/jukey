#include <iostream>

#include "if-trace-mgr.h"
#include "com-factory.h"

using namespace jukey::com;
using namespace jukey::base;

int main(int argc, char** argv)
{
  IComFactory* factory = GetComFactory();
  if (!factory) {
    std::cout << "Get component factory failed!" << std::endl;
    return -1;
  }

  if (!factory->Init("./")) {
    std::cout << "Init component factory failed!" << std::endl;
    return -1;
  }

  ITraceMgr* trace_mgr = (ITraceMgr*)factory->QueryInterface(CID_TRACE_MGR,
		IID_TRACE_MGR, "test");
  if (!trace_mgr) {
    std::cout << "Create trace manager failed!" << std::endl;
    return -1;
  }

  if (!trace_mgr->Init("http://192.168.7.177:4318")) {
    std::cout << "Init trace manager failed!" << std::endl;
    return -1;
  }

  SpanId span_id = trace_mgr->StartSpan("test");
  trace_mgr->StopSpan(span_id);

  return 0;
}