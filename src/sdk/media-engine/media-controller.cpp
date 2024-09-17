#include "media-controller.h"
#include "log.h"

using namespace jukey::com;

namespace jukey::sdk
{

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
//MediaController::MediaController(MediaSrcHolderMgrSP mgr): m_msrc_mgr(mgr)
MediaController::MediaController()
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
MediaState MediaController::State(const com::MediaSrc& msrc)
{
	return MediaState::INVALID;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
uint32_t MediaController::Progress(const com::MediaSrc& msrc)
{
	return 0;
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaController::Start(const com::MediaSrc& msrc)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaController::Pause(const com::MediaSrc& msrc)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaController::Resume(const com::MediaSrc& msrc)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaController::Stop(const com::MediaSrc& msrc)
{

}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaController::SeekBegin(const com::MediaSrc& msrc)
{
	LOG_INF("Seek begin");

	//auto holder = m_msrc_mgr->FindHolder(msrc);
	//if (holder) {
	//	if (ERR_CODE_OK != holder->SeekBegin()) {
	//		LOG_ERR("Seek failed");
	//	}
	//}
	//else {
	//	LOG_ERR("Cannot find holder, media:{}|{}", msrc.src_type, msrc.src_id);
	//}
}

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------
void MediaController::SeekEnd(const com::MediaSrc& msrc, uint32_t prog)
{
	LOG_INF("Seek, progress:{}", prog);

	//auto holder = m_msrc_mgr->FindHolder(msrc);
	//if (holder) {
	//	if (ERR_CODE_OK != holder->SeekEnd(prog)) {
	//		LOG_ERR("Seek failed");
	//	}
	//}
	//else {
	//	LOG_ERR("Cannot find holder, media:{}|{}", msrc.src_type, msrc.src_id);
	//}
}

}