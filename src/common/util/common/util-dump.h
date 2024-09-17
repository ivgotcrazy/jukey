#pragma once

#include <string>
#include <fstream>
#include <memory>

namespace jukey::util
{

//==============================================================================
// 
//==============================================================================
class IDataDumper
{
public:
	virtual ~IDataDumper() {}

	virtual void WriteData(uint8_t* data, uint32_t data_len) = 0;
};
typedef std::shared_ptr<IDataDumper> IDataDumperSP;

//==============================================================================
// 
//==============================================================================
class DataDumper : public IDataDumper
{
public:
	DataDumper(const std::string& file_name);
	~DataDumper();

	// IDataDumper
	virtual void WriteData(uint8_t* data, uint32_t data_len) override;
private:
	std::ofstream* m_ofs = nullptr;
};
typedef std::shared_ptr<DataDumper> DataDumperSP;

}
