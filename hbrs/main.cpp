#include "system/mpp.h"
#include "system/sig_detect.h"
#include "system/pciv_comm.h"
#include "system/pciv_trans.h"
#include "system/vi.h"
#include "system/vo.h"
#include "system/vpss.h"
#include "system/venc.h"
#include "system/vdec.h"
#include "system/ai.h"
#include "system/ao.h"
#include "system/aenc.h"
#include "system/vm.h"
#include "common/buffer.h"
#include "common/config.h"
#include "common/logger.h"
#include "common/http_server.h"
#include "common/json.h"
#include "record/mp4_record.h"
#include "live/rtmp_live.h"

#include "model/record_req.h"
#include "model/local_live_req.h"
#include "model/remote_live_req.h"
#include "model/change_pc_capture_req.h"
#include "model/change_main_scene_req.h"

using namespace rs;

#define CHECK_ERROR(a)                                                                  \
	if (KSuccess != a)                                                                  \
	{                                                                                   \
		log_e("error:%s", make_error_code(static_cast<err_code>(a)).message().c_str()); \
		return a;                                                                       \
	}

static const char *g_Opts = "c:";
struct option g_LongOpts[] = {
	{"config", 1, NULL, 'c'},
	{0, 0, 0, 0}};

static RS_SCENE g_CurMainScene = PC_CAPTURE;

static bool g_Run = true;
static bool g_LiveStart = false;
static bool g_RemoteLiveStart = false;
static bool g_RecordStart = false;
static bool g_MainScreenStart = false;

static void SignalHandler(int signo)
{
	if (signo == SIGINT)
	{
		log_w("recive signal SIGINT,going to shutdown");
		g_Run = false;
	}
	else if (signo == SIGPIPE)
	{
		log_w("receive signal SIGPIPE");
	}
}

static AudioInput ai_main;
static AudioEncode aenc_main;
static AudioOutput ao_main;
static VideoProcess vpss_tea_fea;
static VideoProcess vpss_stu_fea;
static VideoProcess vpss_tea_full;
static VideoProcess vpss_stu_full;
static VideoProcess vpss_black_board;
static VideoProcess vpss_pc;
static VideoProcess vpss_main;
static VideoProcess vpss_tea_fea_2vo;
static VideoProcess vpss_stu_fea_2vo;
static VideoEncode venc_tea_fea;
static VideoEncode venc_stu_fea;
static VideoEncode venc_tea_full;
static VideoEncode venc_stu_full;
static VideoEncode venc_black_board;
static VideoEncode venc_pc;
static VideoEncode venc_main;
static VideoEncode venc_main2;
static VideoDecode vdec_tea_full;
static VideoDecode vdec_stu_full;
static VideoDecode vdec_black_board;
static VideoDecode vdec_pc;
static VideoOutput vo_tea_fea;
static VideoOutput vo_stu_fea;
static VideoOutput vo_main;
static VideoOutput vo_disp;
static MP4Record rec_tea_fea;
static MP4Record rec_stu_fea;
static MP4Record rec_tea_full;
static MP4Record rec_stu_full;
static MP4Record rec_black_board;
static MP4Record rec_pc;
static MP4Record rec_main;
static RTMPLive live_tea_fea;
static RTMPLive live_stu_fea;
static RTMPLive live_tea_full;
static RTMPLive live_stu_full;
static RTMPLive live_black_board;
static RTMPLive live_pc;
static RTMPLive live_main;
static RTMPLive live_main2;

// static int StartMainScreen()
// {
// 	if (g_MainScreenStart)
// 		return KInitialized;

// 	int ret;
// 	std::map<int, std::pair<RECT_S, int>> scene_pos = VideoOutput::GetScenePos(Config::Instance()->scene_.mode);
// 	for (auto it = Config::Instance()->scene_.mapping.begin(); it != Config::Instance()->scene_.mapping.end(); it++)
// 	{
// 		if (scene_pos.count(it->first) == 0)
// 			continue;

// 		ret = vo_main.StartChannel(it->first, scene_pos[it->first].first, scene_pos[it->first].second);
// 		if (ret != KSuccess)
// 			return ret;

// 		switch (it->second)
// 		{
// 		case TEA_FEATURE:
// 			ret = vpss_tea_fea.StartUserChannel(3, scene_pos[it->first].first);
// 			if (ret != KSuccess)
// 				return ret;
// 			break;
// 		case STU_FEATURE:
// 			ret = vpss_stu_fea.StartUserChannel(3, scene_pos[it->first].first);
// 			if (ret != KSuccess)
// 				return ret;
// 			break;
// 		case TEA_FULL_VIEW:
// 			ret = vpss_tea_full.StartUserChannel(3, scene_pos[it->first].first);
// 			if (ret != KSuccess)
// 				return ret;
// 			break;
// 		case STU_FULL_VIEW:
// 			ret = vpss_stu_full.StartUserChannel(3, scene_pos[it->first].first);
// 			if (ret != KSuccess)
// 				return ret;
// 			break;
// 		case BLACK_BOARD_FEATURE:
// 			ret = vpss_black_board.StartUserChannel(3, scene_pos[it->first].first);
// 			if (ret != KSuccess)
// 				return ret;
// 			break;
// 		case PC_CAPTURE:
// 			ret = vpss_pc.StartUserChannel(3, scene_pos[it->first].first);
// 			if (ret != KSuccess)
// 				return ret;
// 			break;
// 		default:
// 			break;
// 		}

// 		if (it->second != MAIN)
// 		{
// 			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(it->second, 3, 12, it->first);
// 			if (ret != KSuccess)
// 				return ret;
// 		}
// 		else
// 		{
// 			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(g_CurMainScene, 4, 12, it->first);
// 			if (ret != KSuccess)
// 				return ret;
// 		}
// 	}

// 	ret = MPPSystem::Bind<HI_ID_VOU, HI_ID_VPSS>(12, 0, 6, 0);
// 	if (ret != KSuccess)
// 		return ret;

// 	g_MainScreenStart = true;
// 	return KSuccess;
// }

// static void CloseMainScreen()
// {
// 	if (!g_MainScreenStart)
// 		return;
// 	MPPSystem::UnBind<HI_ID_VOU, HI_ID_VPSS>(12, 0, 6, 0);

// 	std::map<int, std::pair<RECT_S, int>> scene_pos = VideoOutput::GetScenePos(Config::Instance()->scene_.mode);
// 	for (auto it = Config::Instance()->scene_.mapping.begin(); it != Config::Instance()->scene_.mapping.end(); it++)
// 	{
// 		if (scene_pos.count(it->first) == 0)
// 			continue;

// 		if (it->second != MAIN)
// 		{
// 			MPPSystem::UnBind<HI_ID_VPSS, HI_ID_VOU>(it->second, 3, 12, it->first);
// 		}
// 		else
// 		{
// 			MPPSystem::UnBind<HI_ID_VPSS, HI_ID_VOU>(g_CurMainScene, 4, 12, it->first);
// 		}

// 		switch (it->second)
// 		{
// 		case TEA_FEATURE:
// 			vpss_tea_fea.StopUserChannal(3);
// 			break;
// 		case STU_FEATURE:
// 			vpss_stu_fea.StopUserChannal(3);
// 			break;
// 		case TEA_FULL_VIEW:
// 			vpss_tea_full.StopUserChannal(3);
// 			break;
// 		case STU_FULL_VIEW:
// 			vpss_stu_full.StopUserChannal(3);
// 			break;
// 		case BLACK_BOARD_FEATURE:
// 			vpss_black_board.StopUserChannal(3);
// 			break;
// 		case PC_CAPTURE:
// 			vpss_pc.StopUserChannal(3);
// 			break;
// 		default:
// 			break;
// 		}
// 	}

// 	vo_main.StopAllChn();
// 	g_MainScreenStart = false;
// }

// static int StartDisplayScreen()
// {
// 	int ret;

// 	ret = vo_disp.Initialize({0, VO_INTF_VGA | VO_INTF_HDMI, Config::Instance()->system_.disp_vo_intf_sync});
// 	if (ret != KSuccess)
// 		return ret;

// 	for (auto it = Config::Instance()->system_.chns.begin(); it != Config::Instance()->system_.chns.end(); it++)
// 	{
// 		ret = vo_disp.StartChannel(it->chn, it->rect, 0);
// 		if (ret != KSuccess)
// 			return ret;
// 	}

// 	for (auto it = Config::Instance()->system_.mapping.begin(); it != Config::Instance()->system_.mapping.end(); it++)
// 	{
// 		if (it->second == MAIN)
// 		{
// 			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(6, 2, 0, it->first);
// 			if (ret != KSuccess)
// 				return ret;
// 		}
// 		else
// 		{
// 			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(it->second, 2, 0, it->first);
// 			if (ret != KSuccess)
// 				return ret;
// 		}
// 	}
// 	return KSuccess;
// }

// static void CloseDisplayScreen()
// {
// 	for (auto it = Config::Instance()->system_.mapping.begin(); it != Config::Instance()->system_.mapping.end(); it++)
// 	{
// 		if (it->second == MAIN)
// 		{
// 			MPPSystem::UnBind<HI_ID_VPSS, HI_ID_VOU>(6, 2, 0, it->first);
// 		}
// 		else
// 		{
// 			MPPSystem::UnBind<HI_ID_VPSS, HI_ID_VOU>(it->second, 2, 0, it->first);
// 		}
// 	}
// 	vo_disp.StopAllChn();
// 	vo_disp.Close();
// }

// static int StartVideoEncode()
// {
// 	int ret;
// 	if (Config::Instance()->IsResourceMode())
// 	{
// 		ret = vpss_tea_fea.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.res_width, (HI_U32)Config::Instance()->video_.res_height});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = vpss_stu_fea.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.res_width, (HI_U32)Config::Instance()->video_.res_height});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = vpss_tea_full.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.res_width, (HI_U32)Config::Instance()->video_.res_height});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = vpss_stu_full.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.res_width, (HI_U32)Config::Instance()->video_.res_height});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = vpss_black_board.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.res_width, (HI_U32)Config::Instance()->video_.res_height});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = vpss_pc.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.res_width, (HI_U32)Config::Instance()->video_.res_height});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = vpss_main.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.res_width, (HI_U32)Config::Instance()->video_.res_height});
// 		if (ret != KSuccess)
// 			return ret;

// 		ret = venc_tea_fea.Initialize({0, 0, Config::Instance()->video_.res_width, Config::Instance()->video_.res_height, 25, 25, 0, Config::Instance()->video_.res_bitrate, VENC_RC_MODE_H264CBR, false});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = venc_stu_fea.Initialize({1, 1, Config::Instance()->video_.res_width, Config::Instance()->video_.res_height, 25, 25, 0, Config::Instance()->video_.res_bitrate, VENC_RC_MODE_H264CBR, false});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = venc_tea_full.Initialize({2, 2, Config::Instance()->video_.res_width, Config::Instance()->video_.res_height, 25, 25, 0, Config::Instance()->video_.res_bitrate, VENC_RC_MODE_H264CBR, true});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = venc_stu_full.Initialize({3, 3, Config::Instance()->video_.res_width, Config::Instance()->video_.res_height, 25, 25, 0, Config::Instance()->video_.res_bitrate, VENC_RC_MODE_H264CBR, true});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = venc_black_board.Initialize({4, 4, Config::Instance()->video_.res_width, Config::Instance()->video_.res_height, 25, 25, 0, Config::Instance()->video_.res_bitrate, VENC_RC_MODE_H264CBR, true});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = venc_pc.Initialize({5, 5, Config::Instance()->video_.res_width, Config::Instance()->video_.res_height, 25, 25, 0, Config::Instance()->video_.res_bitrate, VENC_RC_MODE_H264CBR, true});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = venc_main.Initialize({6, 6, Config::Instance()->video_.res_width, Config::Instance()->video_.res_height, 25, 25, 0, Config::Instance()->video_.res_bitrate, VENC_RC_MODE_H264CBR, false});
// 		if (ret != KSuccess)
// 			return ret;

// 		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(0, 1, 0, 0);
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(1, 1, 1, 0);
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(2, 1, 2, 0);
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(3, 1, 3, 0);
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(4, 1, 4, 0);
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(5, 1, 5, 0);
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(6, 1, 6, 0);
// 		if (ret != KSuccess)
// 			return ret;
// 	}
// 	else
// 	{
// 		ret = vpss_tea_fea.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.normal_live_width, (HI_U32)Config::Instance()->video_.normal_live_height});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = vpss_stu_fea.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.normal_live_width, (HI_U32)Config::Instance()->video_.normal_live_height});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = vpss_tea_full.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.normal_live_width, (HI_U32)Config::Instance()->video_.normal_live_height});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = vpss_stu_full.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.normal_live_width, (HI_U32)Config::Instance()->video_.normal_live_height});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = vpss_black_board.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.normal_live_width, (HI_U32)Config::Instance()->video_.normal_live_height});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = vpss_pc.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.normal_live_width, (HI_U32)Config::Instance()->video_.normal_live_height});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = vpss_main.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.normal_record_width, (HI_U32)Config::Instance()->video_.normal_record_height});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = vpss_main.StartUserChannel(3, {0, 0, (HI_U32)Config::Instance()->video_.normal_live_width, (HI_U32)Config::Instance()->video_.normal_live_height});
// 		if (ret != KSuccess)
// 			return ret;

// 		ret = venc_tea_fea.Initialize({0, 0, Config::Instance()->video_.normal_live_width, Config::Instance()->video_.normal_live_height, 25, 25, 0, Config::Instance()->video_.normal_live_bitrate, VENC_RC_MODE_H264CBR, false});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = venc_stu_fea.Initialize({1, 1, Config::Instance()->video_.normal_live_width, Config::Instance()->video_.normal_live_height, 25, 25, 0, Config::Instance()->video_.normal_live_bitrate, VENC_RC_MODE_H264CBR, false});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = venc_tea_full.Initialize({2, 2, Config::Instance()->video_.normal_live_width, Config::Instance()->video_.normal_live_height, 25, 25, 0, Config::Instance()->video_.normal_live_bitrate, VENC_RC_MODE_H264CBR, true});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = venc_stu_full.Initialize({3, 3, Config::Instance()->video_.normal_live_width, Config::Instance()->video_.normal_live_height, 25, 25, 0, Config::Instance()->video_.normal_live_bitrate, VENC_RC_MODE_H264CBR, true});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = venc_black_board.Initialize({4, 4, Config::Instance()->video_.normal_live_width, Config::Instance()->video_.normal_live_height, 25, 25, 0, Config::Instance()->video_.normal_live_bitrate, VENC_RC_MODE_H264CBR, true});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = venc_pc.Initialize({5, 5, Config::Instance()->video_.normal_live_width, Config::Instance()->video_.normal_live_height, 25, 25, 0, Config::Instance()->video_.normal_live_bitrate, VENC_RC_MODE_H264CBR, true});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = venc_main.Initialize({6, 6, Config::Instance()->video_.normal_record_width, Config::Instance()->video_.normal_record_height, 25, 25, 0, Config::Instance()->video_.normal_record_bitrate, VENC_RC_MODE_H264CBR, false});
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = venc_main2.Initialize({7, 7, Config::Instance()->video_.normal_live_width, Config::Instance()->video_.normal_live_height, 25, 25, 0, Config::Instance()->video_.normal_live_bitrate, VENC_RC_MODE_H264CBR, false});
// 		if (ret != KSuccess)
// 			return ret;

// 		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(0, 1, 0, 0);
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(1, 1, 1, 0);
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(2, 1, 2, 0);
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(3, 1, 3, 0);
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(4, 1, 4, 0);
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(5, 1, 5, 0);
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(6, 1, 6, 0);
// 		if (ret != KSuccess)
// 			return ret;
// 		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(6, 3, 7, 0);
// 		if (ret != KSuccess)
// 			return ret;
// 	}
// 	return KSuccess;
// }

// static void CloseVideoEncode()
// {
// 	if (Config::Instance()->IsResourceMode())
// 	{
// 		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(6, 1, 6, 0);
// 		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(5, 1, 5, 0);
// 		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(4, 1, 4, 0);
// 		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(3, 1, 3, 0);
// 		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(2, 1, 2, 0);
// 		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(1, 1, 1, 0);
// 		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(0, 1, 0, 0);

// 		venc_main.Close();
// 		venc_pc.Close();
// 		venc_black_board.Close();
// 		venc_stu_full.Close();
// 		venc_tea_full.Close();
// 		venc_stu_fea.Close();
// 		venc_tea_fea.Close();

// 		vpss_main.StopUserChannal(1);
// 		vpss_pc.StopUserChannal(1);
// 		vpss_black_board.StopUserChannal(1);
// 		vpss_stu_full.StopUserChannal(1);
// 		vpss_tea_full.StopUserChannal(1);
// 		vpss_stu_fea.StopUserChannal(1);
// 		vpss_tea_fea.StopUserChannal(1);
// 	}
// 	else
// 	{
// 		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(6, 3, 7, 0);
// 		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(6, 1, 6, 0);
// 		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(5, 1, 5, 0);
// 		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(4, 1, 4, 0);
// 		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(3, 1, 3, 0);
// 		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(2, 1, 2, 0);
// 		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(1, 1, 1, 0);
// 		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(0, 1, 0, 0);

// 		venc_main2.Close();
// 		venc_main.Close();
// 		venc_pc.Close();
// 		venc_black_board.Close();
// 		venc_stu_full.Close();
// 		venc_tea_full.Close();
// 		venc_stu_fea.Close();
// 		venc_tea_fea.Close();

// 		vpss_main.StopUserChannal(3);
// 		vpss_main.StopUserChannal(1);
// 		vpss_pc.StopUserChannal(1);
// 		vpss_black_board.StopUserChannal(1);
// 		vpss_stu_full.StopUserChannal(1);
// 		vpss_tea_full.StopUserChannal(1);
// 		vpss_stu_fea.StopUserChannal(1);
// 		vpss_tea_fea.StopUserChannal(1);
// 	}
// }

// static int StartRecord(const std::vector<std::pair<RS_SCENE, mp4::Params>> &records)
// {
// 	if (g_RecordStart)
// 		return KInitialized;
// 	int ret;
// 	for (const std::pair<RS_SCENE, mp4::Params> &rec : records)
// 	{
// 		switch (rec.first)
// 		{
// 		case TEA_FEATURE:
// 		{
// 			if (!Config::Instance()->IsResourceMode())
// 				continue;
// 			ret = rec_tea_fea.Initialize(rec.second);
// 			if (ret != KSuccess)
// 				return ret;
// 			aenc_main.AddAudioSink(&rec_tea_fea);
// 			venc_tea_fea.AddVideoSink(&rec_tea_fea);
// 			break;
// 		}
// 		case STU_FEATURE:
// 		{
// 			if (!Config::Instance()->IsResourceMode())
// 				continue;
// 			ret = rec_stu_fea.Initialize(rec.second);
// 			if (ret != KSuccess)
// 				return ret;
// 			aenc_main.AddAudioSink(&rec_stu_fea);
// 			venc_stu_fea.AddVideoSink(&rec_stu_fea);
// 			break;
// 		}

// 		case TEA_FULL_VIEW:
// 		{
// 			if (!Config::Instance()->IsResourceMode())
// 				continue;
// 			ret = rec_tea_full.Initialize(rec.second);
// 			if (ret != KSuccess)
// 				return ret;
// 			aenc_main.AddAudioSink(&rec_tea_full);
// 			venc_tea_full.AddVideoSink(&rec_tea_full);
// 			break;
// 		}
// 		case STU_FULL_VIEW:
// 		{
// 			if (!Config::Instance()->IsResourceMode())
// 				continue;
// 			ret = rec_stu_full.Initialize(rec.second);
// 			if (ret != KSuccess)
// 				return ret;
// 			aenc_main.AddAudioSink(&rec_stu_full);
// 			venc_stu_full.AddVideoSink(&rec_stu_full);
// 			break;
// 		}
// 		case BLACK_BOARD_FEATURE:
// 		{
// 			if (!Config::Instance()->IsResourceMode())
// 				continue;
// 			ret = rec_black_board.Initialize(rec.second);
// 			if (ret != KSuccess)
// 				return ret;
// 			aenc_main.AddAudioSink(&rec_black_board);
// 			venc_black_board.AddVideoSink(&rec_black_board);
// 			break;
// 		}
// 		case PC_CAPTURE:
// 		{
// 			if (!Config::Instance()->IsResourceMode())
// 				continue;
// 			ret = rec_pc.Initialize(rec.second);
// 			if (ret != KSuccess)
// 				return ret;
// 			aenc_main.AddAudioSink(&rec_pc);
// 			venc_pc.AddVideoSink(&rec_pc);
// 			break;
// 		}
// 		case MAIN:
// 		{
// 			ret = rec_main.Initialize(rec.second);
// 			if (ret != KSuccess)
// 				return ret;
// 			aenc_main.AddAudioSink(&rec_main);
// 			venc_main.AddVideoSink(&rec_main);
// 			break;
// 		}
// 		default:
// 			break;
// 		}
// 	}
// 	g_RecordStart = true;
// 	return KSuccess;
// }

// static void CloseRecord()
// {
// 	if (!g_RecordStart)
// 		return;
// 	aenc_main.RemoveAudioSink(&rec_tea_fea);
// 	venc_tea_fea.RemoveVideoSink(&rec_tea_fea);
// 	aenc_main.RemoveAudioSink(&rec_stu_fea);
// 	venc_stu_fea.RemoveVideoSink(&rec_stu_fea);
// 	aenc_main.RemoveAudioSink(&rec_tea_full);
// 	venc_tea_full.RemoveVideoSink(&rec_tea_full);
// 	aenc_main.RemoveAudioSink(&rec_stu_full);
// 	venc_stu_full.RemoveVideoSink(&rec_stu_full);
// 	aenc_main.RemoveAudioSink(&rec_black_board);
// 	venc_black_board.RemoveVideoSink(&rec_black_board);
// 	aenc_main.RemoveAudioSink(&rec_pc);
// 	venc_pc.RemoveVideoSink(&rec_pc);
// 	aenc_main.RemoveAudioSink(&rec_main);
// 	venc_main.RemoveVideoSink(&rec_main);

// 	rec_tea_fea.Close();
// 	rec_stu_fea.Close();
// 	rec_tea_full.Close();
// 	rec_stu_full.Close();
// 	rec_black_board.Close();
// 	rec_pc.Close();
// 	rec_main.Close();

// 	g_RecordStart = false;
// }

// static int StartLive(const std::vector<std::pair<RS_SCENE, rtmp::Params>> &lives)
// {
// 	if (g_LiveStart)
// 		return KInitialized;
// 	int ret;
// 	for (const std::pair<RS_SCENE, rtmp::Params> &live : lives)
// 	{
// 		switch (live.first)
// 		{
// 		case TEA_FEATURE:
// 		{
// 			ret = live_tea_fea.Initialize(live.second);
// 			if (ret != KSuccess)
// 				return ret;
// 			venc_tea_fea.AddVideoSink(&live_tea_fea);
// 			break;
// 		}
// 		case STU_FEATURE:
// 		{
// 			ret = live_stu_fea.Initialize(live.second);
// 			if (ret != KSuccess)
// 				return ret;
// 			venc_stu_fea.AddVideoSink(&live_stu_fea);
// 			break;
// 		}

// 		case TEA_FULL_VIEW:
// 		{
// 			ret = live_tea_full.Initialize(live.second);
// 			if (ret != KSuccess)
// 				return ret;
// 			venc_tea_full.AddVideoSink(&live_tea_full);
// 			break;
// 		}
// 		case STU_FULL_VIEW:
// 		{
// 			ret = live_stu_full.Initialize(live.second);
// 			if (ret != KSuccess)
// 				return ret;
// 			venc_stu_full.AddVideoSink(&live_stu_full);
// 			break;
// 		}
// 		case BLACK_BOARD_FEATURE:
// 		{
// 			ret = live_black_board.Initialize(live.second);
// 			if (ret != KSuccess)
// 				return ret;
// 			venc_black_board.AddVideoSink(&live_black_board);
// 			break;
// 		}
// 		case PC_CAPTURE:
// 		{
// 			ret = live_pc.Initialize(live.second);
// 			if (ret != KSuccess)
// 				return ret;
// 			venc_pc.AddVideoSink(&live_pc);
// 			break;
// 		}
// 		case MAIN:
// 		{
// 			ret = live_main.Initialize(live.second);
// 			if (ret != KSuccess)
// 				return ret;
// 			aenc_main.AddAudioSink(&live_main);
// 			if (Config::Instance()->IsResourceMode())
// 			{
// 				venc_main.AddVideoSink(&live_main);
// 			}
// 			else
// 			{
// 				venc_main2.AddVideoSink(&live_main);
// 			}
// 			break;
// 		}
// 		default:
// 			break;
// 		}
// 	}
// 	g_LiveStart = true;
// 	return KSuccess;
// }

// void CloseLive()
// {
// 	if (!g_LiveStart)
// 		return;
// 	venc_tea_fea.RemoveVideoSink(&live_tea_fea);
// 	live_tea_fea.Close();
// 	venc_stu_fea.RemoveVideoSink(&live_stu_fea);
// 	live_stu_fea.Close();
// 	venc_tea_full.RemoveVideoSink(&live_tea_full);
// 	live_tea_full.Close();
// 	venc_stu_full.RemoveVideoSink(&live_stu_full);
// 	live_stu_full.Close();
// 	venc_black_board.RemoveVideoSink(&live_black_board);
// 	live_black_board.Close();
// 	venc_pc.RemoveVideoSink(&live_pc);
// 	live_pc.Close();

// 	aenc_main.RemoveAudioSink(&live_main);
// 	if (Config::Instance()->IsResourceMode())
// 	{
// 		venc_main.RemoveVideoSink(&live_main);
// 	}
// 	else
// 	{
// 		venc_main2.RemoveVideoSink(&live_main);
// 	}
// 	live_main.Close();
// 	g_LiveStart = false;
// }

// int StartRemoteLive(const rtmp::Params &params)
// {
// 	if (g_RemoteLiveStart)
// 		return KInitialized;
// 	int ret;
// 	ret = live_main2.Initialize(params);
// 	if (ret != KSuccess)
// 		return ret;
// 	aenc_main.AddAudioSink(&live_main2);
// 	if (Config::Instance()->IsResourceMode())
// 	{
// 		venc_main.AddVideoSink(&live_main2);
// 	}
// 	else
// 	{
// 		venc_main2.AddVideoSink(&live_main2);
// 	}

// 	g_RemoteLiveStart = true;
// 	return KSuccess;
// }

// void CloseRemoteLive()
// {
// 	if (!g_RemoteLiveStart)
// 		return;

// 	aenc_main.RemoveAudioSink(&live_main2);
// 	if (Config::Instance()->IsResourceMode())
// 	{
// 		venc_main.RemoveVideoSink(&live_main2);
// 	}
// 	else
// 	{
// 		venc_main2.RemoveVideoSink(&live_main2);
// 	}
// 	live_main2.Close();
// 	g_RemoteLiveStart = false;
// }

// static void StartRecordHandler(evhttp_request *req, void *arg)
// {
// 	int ret;

// 	std::string str = HttpServer::GetRequestData(req);
// 	log_d("request body:%s", str.c_str());

// 	Json::Value root;
// 	if (JsonUtils::toJson(str, root) != KSuccess)
// 	{
// 		HttpServer::MakeResponse(req, HTTP_SERVUNAVAIL, "format error", "{\"errMsg\":\"parse json root failed\"}");
// 		log_w("parse json root failed");
// 		return;
// 	}

// 	if (!RecordReq::IsOk(root))
// 	{
// 		HttpServer::MakeResponse(req, HTTP_SERVUNAVAIL, "format error", "{\"errMsg\":\"check json format failed\"}");
// 		log_w("check json format failed");
// 		return;
// 	}

// 	RecordReq record_req;
// 	record_req = root;

// 	CloseRecord();
// 	ret = StartRecord(record_req.records.recs);
// 	if (ret != KSuccess)
// 	{
// 		HttpServer::MakeResponse(req, HTTP_INTERNAL, "system error", "{\"errMsg\":\"start record failed\"}");
// 		log_w("start record failed");
// 		return;
// 	}

// 	HttpServer::MakeResponse(req, HTTP_OK, "ok", "{\"errMsg\":\"success\"}");
// 	log_d("request ok");
// }

// static void StopRecordHandler(evhttp_request *req, void *arg)
// {
// 	CloseRecord();
// 	HttpServer::MakeResponse(req, HTTP_OK, "ok", "{\"errMsg\":\"success\"}");
// 	log_d("request ok");
// }

// static void StartLiveHandler(evhttp_request *req, void *arg)
// {
// 	int ret;

// 	std::string str = HttpServer::GetRequestData(req);
// 	log_d("request body:%s", str.c_str());

// 	Json::Value root;
// 	if (JsonUtils::toJson(str, root) != KSuccess)
// 	{
// 		HttpServer::MakeResponse(req, HTTP_SERVUNAVAIL, "format error", "{\"errMsg\":\"parse json root failed\"}");
// 		log_w("parse json root failed");
// 		return;
// 	}

// 	if (!LocalLiveReq::IsOk(root))
// 	{
// 		HttpServer::MakeResponse(req, HTTP_SERVUNAVAIL, "format error", "{\"errMsg\":\"check json format failed\"}");
// 		log_w("check json format failed");
// 		return;
// 	}

// 	LocalLiveReq live_req;
// 	live_req = root;

// 	CloseLive();
// 	ret = StartLive(live_req.local_lives.lives);
// 	if (ret != KSuccess)
// 	{
// 		HttpServer::MakeResponse(req, HTTP_INTERNAL, "system error", "{\"errMsg\":\"start live failed\"}");
// 		log_w("start live failed");
// 		return;
// 	}

// 	HttpServer::MakeResponse(req, HTTP_OK, "ok", "{\"errMsg\":\"success\"}");
// 	log_d("request ok");

// 	Config::Instance()->local_lives_ = live_req.local_lives;
// 	Config::Instance()->WriteToFile();
// }

// static void StopLiveHandler(evhttp_request *req, void *arg)
// {
// 	CloseLive();
// 	HttpServer::MakeResponse(req, HTTP_OK, "ok", "{\"errMsg\":\"success\"}");
// 	log_d("request ok");

// 	Config::Instance()->local_lives_.lives = {};
// 	Config::Instance()->WriteToFile();
// }

// static void StartRemoteLiveHandler(evhttp_request *req, void *arg)
// {
// 	int ret;

// 	std::string str = HttpServer::GetRequestData(req);
// 	log_d("request body:%s", str.c_str());

// 	Json::Value root;
// 	if (JsonUtils::toJson(str, root) != KSuccess)
// 	{
// 		HttpServer::MakeResponse(req, HTTP_SERVUNAVAIL, "format error", "{\"errMsg\":\"parse json root failed\"}");
// 		log_w("parse json root failed");
// 		return;
// 	}

// 	if (!RemoteLiveReq::IsOk(root))
// 	{
// 		HttpServer::MakeResponse(req, HTTP_SERVUNAVAIL, "format error", "{\"errMsg\":\"check json format failed\"}");
// 		log_w("check json format failed");
// 		return;
// 	}

// 	RemoteLiveReq remote_live_req;
// 	remote_live_req = root;

// 	CloseRemoteLive();
// 	ret = StartRemoteLive(remote_live_req.remote_live.live);
// 	if (ret != KSuccess)
// 	{
// 		HttpServer::MakeResponse(req, HTTP_INTERNAL, "system error", "{\"errMsg\":\"start remote live failed\"}");
// 		log_w("start remote live failed");
// 		return;
// 	}

// 	HttpServer::MakeResponse(req, HTTP_OK, "ok", "{\"errMsg\":\"success\"}");
// 	log_d("request ok");

// 	Config::Instance()->remote_live_ = remote_live_req.remote_live;
// 	Config::Instance()->WriteToFile();
// }

// static void StopRemoteLiveHandler(evhttp_request *req, void *arg)
// {
// 	CloseRemoteLive();
// 	HttpServer::MakeResponse(req, HTTP_OK, "ok", "{\"errMsg\":\"success\"}");
// 	log_d("request ok");

// 	Config::Instance()->remote_live_.live.url = "";
// 	Config::Instance()->WriteToFile();
// }

// static void ChangePCCaptureHandler(evhttp_request *req, void *arg)
// {
// 	int ret;

// 	std::string str = HttpServer::GetRequestData(req);
// 	log_d("request body:%s", str.c_str());

// 	Json::Value root;
// 	if (JsonUtils::toJson(str, root) != KSuccess)
// 	{
// 		HttpServer::MakeResponse(req, HTTP_SERVUNAVAIL, "format error", "{\"errMsg\":\"parse json root failed\"}");
// 		log_w("parse json root failed");
// 		return;
// 	}

// 	if (!ChangePCCaptureReq::IsOk(root))
// 	{
// 		HttpServer::MakeResponse(req, HTTP_SERVUNAVAIL, "format error", "{\"errMsg\":\"check json format failed\"}");
// 		log_w("check json format failed");
// 		return;
// 	}
// 	ChangePCCaptureReq change_pc_capture_req;
// 	change_pc_capture_req = root;

// 	Config::Instance()->system_.pc_capture_mode = change_pc_capture_req.mode;
// 	// ret = SigDetect::Instance()->SetPCCaptureMode(change_pc_capture_req.mode);
// 	// if (ret != KSuccess)
// 	// {
// 	// 	HttpServer::MakeResponse(req, HTTP_INTERNAL, "system error", "{\"errMsg\":\"change pc capture mode failed\"}");
// 	// 	log_w("change pc capture mode failed");
// 	// 	return;
// 	// }

// 	HttpServer::MakeResponse(req, HTTP_OK, "ok", "{\"errMsg\":\"success\"}");
// 	log_d("request ok");

// 	Config::Instance()->WriteToFile();
// }

// static void ChangeMainScreenHandler(evhttp_request *req, void *arg)
// {
// 	int ret;

// 	std::string str = HttpServer::GetRequestData(req);
// 	log_d("request body:%s", str.c_str());

// 	Json::Value root;
// 	if (JsonUtils::toJson(str, root) != KSuccess)
// 	{
// 		HttpServer::MakeResponse(req, HTTP_SERVUNAVAIL, "format error", "{\"errMsg\":\"parse json root failed\"}");
// 		log_w("parse json root failed");
// 		return;
// 	}

// 	if (!ChangeMainScreenReq::IsOk(root))
// 	{
// 		HttpServer::MakeResponse(req, HTTP_SERVUNAVAIL, "format error", "{\"errMsg\":\"check json format failed\"}");
// 		log_w("check json format failed");
// 		return;
// 	}

// 	ChangeMainScreenReq change_main_screen_req;
// 	change_main_screen_req = root;

// 	CloseRecord();
// 	CloseLive();
// 	CloseRemoteLive();
// 	CloseMainScreen();

// 	if (Config::IsResourceMode(change_main_screen_req.scene.mode) != Config::Instance()->IsResourceMode())
// 	{
// 		log_d("need to restart video encode module");
// 		CloseVideoEncode();
// 		Config::Instance()->scene_.mode = change_main_screen_req.scene.mode;
// 		ret = StartVideoEncode();
// 		if (ret != KSuccess)
// 		{
// 			HttpServer::MakeResponse(req, HTTP_INTERNAL, "system error", "{\"errMsg\":\"start video encode failed\"}");
// 			log_w("start video encode failed");
// 			return;
// 		}
// 	}

// 	Config::Instance()->scene_ = change_main_screen_req.scene;

// 	ret = StartMainScreen();
// 	if (ret != KSuccess)
// 	{
// 		HttpServer::MakeResponse(req, HTTP_INTERNAL, "system error", "{\"errMsg\":\"start main screen failed\"}");
// 		log_w("start main screen failed");
// 		return;
// 	}

// 	if (!Config::Instance()->local_lives_.lives.empty())
// 	{
// 		ret = StartLive(Config::Instance()->local_lives_.lives);
// 		if (ret != KSuccess)
// 		{
// 			HttpServer::MakeResponse(req, HTTP_INTERNAL, "system error", "{\"errMsg\":\"start local live failed\"}");
// 			log_w("start local live failed");
// 			return;
// 		}
// 	}

// 	if (Config::Instance()->remote_live_.live.url != "")
// 	{
// 		ret = StartRemoteLive(Config::Instance()->remote_live_.live);
// 		if (ret != KSuccess)
// 		{
// 			HttpServer::MakeResponse(req, HTTP_INTERNAL, "system error", "{\"errMsg\":\"start remote live failed\"}");
// 			log_w("start remote live failed");
// 			return;
// 		}
// 	}

// 	HttpServer::MakeResponse(req, HTTP_OK, "ok", "{\"errMsg\":\"success\"}");
// 	log_d("request ok");
// 	Config::Instance()->WriteToFile();
// }

int32_t main(int32_t argc, char **argv)
{
	int ret;

	ConfigLogger();

	signal(SIGINT, SignalHandler);
	signal(SIGPIPE, SignalHandler);

	bool got_config_file = false;
	int opt;
	while ((opt = getopt_long(argc, argv, g_Opts, g_LongOpts, NULL)) != -1)
	{
		switch (opt)
		{
		case 'c':
		{
			log_w("using config file %s", optarg);
			ret = Config::Instance()->Initialize(optarg);
			CHECK_ERROR(ret);
			got_config_file = true;
			break;
		}
		default:
		{
			log_w("unknow argument:%c", opt);
			break;
		}
		}
	}

	if (!got_config_file)
	{
		log_w("Usage:%s -c [conf_file_path]", argv[0]);
		//休眠让日志有足够的时间输出
		usleep(100000); //100ms
		return 0;
	}

	ret = MPPSystem::Instance()->Initialize();
	CHECK_ERROR(ret);

	VideoManager vm;
	vm.Initialize();



// 	ret = ai_main.Initialize({4, 0});
// 	CHECK_ERROR(ret);

// 	ret = aenc_main.Initialize();
// 	CHECK_ERROR(ret);

// 	ret = ao_main.Initialize({4, 0});
// 	CHECK_ERROR(ret);

// 	ret = PCIVComm::Instance()->Initialize();
// 	CHECK_ERROR(ret);
// 	ret = PCIVTrans::Instance()->Initialize(PCIVComm::Instance());
// 	CHECK_ERROR(ret);

// 	ret = vdec_tea_full.Initialize({0, RS_MAX_WIDTH, RS_MAX_HEIGHT});
// 	CHECK_ERROR(ret);
// 	ret = vdec_stu_full.Initialize({1, RS_MAX_WIDTH, RS_MAX_HEIGHT});
// 	CHECK_ERROR(ret);
// 	ret = vdec_black_board.Initialize({2, RS_MAX_WIDTH, RS_MAX_WIDTH});
// 	CHECK_ERROR(ret);
// 	ret = vdec_pc.Initialize({3, RS_MAX_WIDTH, RS_MAX_HEIGHT});
// 	CHECK_ERROR(ret);

// 	ret = vpss_tea_fea_2vo.Initialize({10});
// 	CHECK_ERROR(ret);
// 	ret = vpss_stu_fea_2vo.Initialize({11});
// 	CHECK_ERROR(ret);
// 	ret = vpss_tea_fea.Initialize({0});
// 	CHECK_ERROR(ret);
// 	ret = vpss_stu_fea.Initialize({1});
// 	CHECK_ERROR(ret);
// 	ret = vpss_tea_full.Initialize({2});
// 	CHECK_ERROR(ret);
// 	ret = vpss_stu_full.Initialize({3});
// 	CHECK_ERROR(ret);
// 	ret = vpss_black_board.Initialize({4});
// 	CHECK_ERROR(ret);
// 	ret = vpss_pc.Initialize({5});
// 	CHECK_ERROR(ret);
// 	ret = vpss_main.Initialize({6});
// 	CHECK_ERROR(ret);

// 	ret = vo_tea_fea.Initialize({10, 0, VO_OUTPUT_1080P25});
// 	CHECK_ERROR(ret);
// 	ret = vo_stu_fea.Initialize({11, 0, VO_OUTPUT_1080P25});
// 	CHECK_ERROR(ret);
// 	ret = vo_main.Initialize({12, 0, VO_OUTPUT_1080P25});
// 	CHECK_ERROR(ret);

// 	ret = SigDetect::Instance()->Initialize(PCIVComm::Instance(), Config::Instance()->system_.pc_capture_mode);
// 	CHECK_ERROR(ret);

// 	VIHelper vi_tea_fea(4, 8, &vo_tea_fea);
// 	VIHelper vi_stu_fea(2, 4, &vo_stu_fea);
// 	ret = vo_tea_fea.StartChannel(0, {0, 0, RS_MAX_WIDTH, RS_MAX_HEIGHT}, 0);
// 	CHECK_ERROR(ret);
// 	ret = vo_stu_fea.StartChannel(0, {0, 0, RS_MAX_WIDTH, RS_MAX_HEIGHT}, 0);
// 	CHECK_ERROR(ret);
// 	ai_main.AddAudioSink(&aenc_main);

// 	SigDetect::Instance()->AddVIFmtListener(&vi_tea_fea);
// 	SigDetect::Instance()->AddVIFmtListener(&vi_stu_fea);
// 	PCIVTrans::Instance()->AddVideoSink(&vdec_tea_full);
// 	PCIVTrans::Instance()->AddVideoSink(&vdec_stu_full);
// 	PCIVTrans::Instance()->AddVideoSink(&vdec_black_board);
// 	PCIVTrans::Instance()->AddVideoSink(&vdec_pc);

// 	ret = MPPSystem::Bind<HI_ID_VIU, HI_ID_VPSS>(0, 8, 10, 0);
// 	CHECK_ERROR(ret);
// 	ret = MPPSystem::Bind<HI_ID_VIU, HI_ID_VPSS>(0, 4, 11, 0);
// 	CHECK_ERROR(ret);
// 	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(10, 4, 10, 0);
// 	CHECK_ERROR(ret);
// 	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(11, 4, 11, 0);
// 	CHECK_ERROR(ret);
// 	ret = MPPSystem::Bind<HI_ID_VOU, HI_ID_VPSS>(10, 0, 0, 0);
// 	CHECK_ERROR(ret);
// 	ret = MPPSystem::Bind<HI_ID_VOU, HI_ID_VPSS>(11, 0, 1, 0);
// 	CHECK_ERROR(ret);
// 	ret = MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 0, 2, 0);
// 	CHECK_ERROR(ret);
// 	ret = MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 1, 3, 0);
// 	CHECK_ERROR(ret);
// 	ret = MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 2, 4, 0);
// 	CHECK_ERROR(ret);
// 	ret = MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 3, 5, 0);
// 	CHECK_ERROR(ret);
// 	ret = MPPSystem::Bind<HI_ID_AI, HI_ID_AO>(4, 0, 4, 0);
// 	CHECK_ERROR(ret);

	// ret = StartVideoEncode();
	// CHECK_ERROR(ret);

// 	ret = StartDisplayScreen();
// 	CHECK_ERROR(ret);

// 	ret = StartMainScreen();
// 	CHECK_ERROR(ret);

// 	ret = StartLive(Config::Instance()->local_lives_.lives);
// 	CHECK_ERROR(ret);

// 	if (Config::Instance()->remote_live_.live.url != "")
// 	{
// 		ret = StartRemoteLive(Config::Instance()->remote_live_.live);
// 		CHECK_ERROR(ret);
// 	}

	HttpServer http_server;
	http_server.Initialize("0.0.0.0", 8081);
// 	http_server.RegisterURI("/start_record", StartRecordHandler, nullptr);
// 	http_server.RegisterURI("/stop_record", StopRecordHandler, nullptr);
// 	http_server.RegisterURI("/start_local_live", StartLiveHandler, nullptr);
// 	http_server.RegisterURI("/stop_local_live", StopLiveHandler, nullptr);
// 	http_server.RegisterURI("/start_remote_live", StartRemoteLiveHandler, nullptr);
// 	http_server.RegisterURI("/stop_remote_live", StopRemoteLiveHandler, nullptr);
// 	http_server.RegisterURI("/change_pc_capture", ChangePCCaptureHandler, nullptr);
// 	http_server.RegisterURI("/change_main_screen", ChangeMainScreenHandler, nullptr);

// #if 0
// 	ChangeMainScreenReq test_req;
// 	test_req.mode = Config::Instance()->scene_.mode;
// 	test_req.mapping = Config::Instance()->scene_.mapping;

// 	Json::Value test_json = test_req;
// 	std::string test_str = JsonUtils::toStr(test_json);
// 		printf("test_req:%s\n", test_str.c_str());

// 	Json::Value test_json2;
// 	if (JsonUtils::toJson(test_str, test_json2) == 0)
// 	{
// 		if (ChangeMainScreenReq::IsOk(test_json2))
// 		{
// 			std::string test_str2 = JsonUtils::toStr(test_json2);
// 			printf("#####:%s\n",test_str2.c_str());
// 		}
// 	}
// #endif

// 	Config::Instance()->WriteToFile();
	while (g_Run)
		http_server.Dispatch();

	vm.Close();
// 	CloseRecord();
// 	CloseLive();
// 	CloseRemoteLive();
// 	CloseMainScreen();
// 	CloseDisplayScreen();
// 	CloseVideoEncode();

// 	SigDetect::Instance()->Close();
// 	vo_main.Close();
// 	vo_stu_fea.Close();
// 	vo_tea_fea.Close();
// 	vpss_main.Close();
// 	vpss_pc.Close();
// 	vpss_black_board.Close();
// 	vpss_stu_full.Close();
// 	vpss_tea_full.Close();
// 	vpss_stu_fea.Close();
// 	vpss_tea_fea.Close();
// 	vpss_stu_fea_2vo.Close();
// 	vpss_tea_fea_2vo.Close();
// 	vdec_pc.Close();
// 	vdec_black_board.Close();
// 	vdec_stu_full.Close();
// 	vdec_tea_full.Close();
// 	PCIVTrans::Instance()->Close();
// 	PCIVComm::Instance()->Close();
// 	ao_main.Close();
// 	aenc_main.Close();
// 	ai_main.Close();
// 	MPPSystem::Instance()->Close();

	return 0;
}
