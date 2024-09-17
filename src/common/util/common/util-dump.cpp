#include <fstream>

#include "util-dump.h"
#include "log/util-log.h"
#include "common/util-common.h"


namespace jukey::util
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
DataDumper::DataDumper(const std::string& file_name)
{
	m_ofs = new std::ofstream(file_name, std::ios_base::binary);
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
DataDumper::~DataDumper()
{
	if (m_ofs) {
		m_ofs->close();
		delete m_ofs;
	}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void DataDumper::WriteData(uint8_t* data, uint32_t data_len)
{
	if (m_ofs) {
		m_ofs->write((char*)data, data_len);
		m_ofs->flush();
	}
}

}
