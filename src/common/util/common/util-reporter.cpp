#include "util-reporter.h"
#include "httplib.h"
#include "log/util-log.h"
#include "sig-msg-builder.h"
#include "protoc/topo.pb.h"
#include "protocol.h"
#include "common/util-time.h"


namespace jukey::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
HttpReportSender::HttpReportSender(const std::string& host, uint16_t port, 
  const std::string& path)
  : m_host(host), m_port(port), m_path(path)
{
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void HttpReportSender::SendReport(const std::string& data)
{
  httplib::Client cli(m_host, m_port);

  auto res = cli.Post(m_path, data, "application/json");
  if (res && res->status != 200) {
    UTIL_ERR("send report failed, data:{}, status:{}", data, res->status);
  }
}

}