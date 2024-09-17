#include "setting-mgr.h"
#include "sqlite3pp.h"
#include "log.h"


namespace jukey::demo
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
SettingMgr& SettingMgr::Instance()
{
	static SettingMgr mgr;
	return mgr;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
bool SettingMgr::Init(const std::string& db_file)
{
	m_db_file = db_file;

	sqlite3pp::database db(db_file.c_str());
	if (db.error_code() != SQLITE_OK) {
		LOG_ERR("Load db file failed, error:{}", db.error_msg());
		return false;
	}

	db.execute("CREATE TABLE IF NOT EXISTS demo("
		"curr_spk VARCHAR(256) NOT NULL,"
		"curr_mic VARCHAR(256) NOT NULL,"
		"sample_rate INT NOT NULL,"
		"sample_chnl INT NOT NULL,"
		"sample_bits INT NOT NULL,"
		"curr_cam VARCHAR(256) NOT NULL,"
		"resolution INT NOT NULL,"
		"pixel_format INT NOT NULL,"
		"frame_rate INT NOT NULL);");
	if (db.error_code() != SQLITE_OK) {
		LOG_ERR("Create table failed, error:{}", db.error_msg());
		return false;
	}

	sqlite3pp::query qry(db, "SELECT * FROM demo");
	if (db.error_code() != SQLITE_OK) {
		LOG_ERR("Query from demo table failed, error:{}", db.error_msg());
		return false;
	}

	for (sqlite3pp::query::iterator i = qry.begin(); i != qry.end(); ++i) {
		m_demo_setting.spk_setting.curr_spk = (*i).get<int>(0);
		m_demo_setting.mic_setting.curr_mic = (*i).get<int>(1);
		m_demo_setting.mic_setting.sample_rate = (*i).get<int>(2);
		m_demo_setting.mic_setting.sample_chnl = (*i).get<int>(3);
		m_demo_setting.mic_setting.sample_bits = (*i).get<int>(4);
		m_demo_setting.cam_setting.curr_cam = (*i).get<int>(5);
		m_demo_setting.cam_setting.resolution = (*i).get<int>(6);
		m_demo_setting.cam_setting.pixel_format = (*i).get<int>(7);
		m_demo_setting.cam_setting.frame_rate = (*i).get<int>(8);
	}

	return true;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void SettingMgr::UpdateSetting()
{
	sqlite3pp::database db(m_db_file.c_str());
	if (db.error_code() != SQLITE_OK) {
		LOG_ERR("Load db file failed, file:{}, error:{}", m_db_file, db.error_msg());
		return;
	}

	sqlite3pp::query qry(db, "DELETE FROM demo");
	if (db.error_code() != SQLITE_OK) {
		LOG_ERR("Delete data failed, error:{}", db.error_msg());
		return;
	}

	const char* sql = "INSERT INTO demo VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

	sqlite3pp::command cmd(db, sql);
	cmd.bind(1, (int)m_demo_setting.spk_setting.curr_spk);
	cmd.bind(2, (int)m_demo_setting.mic_setting.curr_mic);
	cmd.bind(3, (int)m_demo_setting.mic_setting.sample_rate);
	cmd.bind(4, (int)m_demo_setting.mic_setting.sample_chnl);
	cmd.bind(5, (int)m_demo_setting.mic_setting.sample_bits);
	cmd.bind(6, (int)m_demo_setting.cam_setting.curr_cam);
	cmd.bind(7, (int)m_demo_setting.cam_setting.resolution);
	cmd.bind(8, (int)m_demo_setting.cam_setting.pixel_format);
	cmd.bind(9, (int)m_demo_setting.cam_setting.frame_rate);

	if (SQLITE_OK != cmd.execute()) {
		LOG_ERR("Execute failed, error:{}", db.error_msg());
	}
}

}