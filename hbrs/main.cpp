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
#include "common/buffer.h"
#include "common/config.h"
#include "common/logger.h"
#include "common/http_server.h"
#include "common/json.h"
#include "record/mp4_record.h"
#include "live/rtmp_live.h"
#include "model/record_req.h"
#include "model/live_req.h"

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

static int StartMainScreen()
{
	int ret;
	std::map<int, std::pair<RECT_S, int>> scene_pos = VideoOutput::GetScenePos(Config::Instance()->scene_.mode);
	for (auto it = Config::Instance()->scene_.mapping.begin(); it != Config::Instance()->scene_.mapping.end(); it++)
	{
		if (scene_pos.count(it->first) == 0)
			continue;

		ret = vo_main.StartChannel(it->first, scene_pos[it->first].first, scene_pos[it->first].second);
		if (ret != KSuccess)
			return ret;

		switch (it->second)
		{
		case TEA_FEATURE:
			ret = vpss_tea_fea.StartUserChannel(3, scene_pos[it->first].first);
			if (ret != KSuccess)
				return ret;
			break;
		case STU_FEATURE:
			ret = vpss_stu_fea.StartUserChannel(3, scene_pos[it->first].first);
			if (ret != KSuccess)
				return ret;
			break;
		case TEA_FULL_VIEW:
			ret = vpss_tea_full.StartUserChannel(3, scene_pos[it->first].first);
			if (ret != KSuccess)
				return ret;
			break;
		case STU_FULL_VIEW:
			ret = vpss_stu_full.StartUserChannel(3, scene_pos[it->first].first);
			if (ret != KSuccess)
				return ret;
			break;
		case BLACK_BOARD_FEATURE:
			ret = vpss_black_board.StartUserChannel(3, scene_pos[it->first].first);
			if (ret != KSuccess)
				return ret;
			break;
		case PC_CAPTURE:
			ret = vpss_pc.StartUserChannel(3, scene_pos[it->first].first);
			if (ret != KSuccess)
				return ret;
			break;
		default:
			break;
		}

		if (it->second != MAIN)
		{
			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(it->second, 3, 12, it->first);
			if (ret != KSuccess)
				return ret;
		}
		else
		{
			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(g_CurMainScene, 4, 12, it->first);
			if (ret != KSuccess)
				return ret;
		}
	}

	ret = MPPSystem::Bind<HI_ID_VOU, HI_ID_VPSS>(12, 0, 6, 0);
	if (ret != KSuccess)
		return ret;

	return KSuccess;
}

static void CloseMainScreen()
{
	MPPSystem::UnBind<HI_ID_VOU, HI_ID_VPSS>(12, 0, 6, 0);

	std::map<int, std::pair<RECT_S, int>> scene_pos = VideoOutput::GetScenePos(Config::Instance()->scene_.mode);
	for (auto it = Config::Instance()->scene_.mapping.begin(); it != Config::Instance()->scene_.mapping.end(); it++)
	{
		if (scene_pos.count(it->first) == 0)
			continue;

		if (it->second != MAIN)
		{
			MPPSystem::UnBind<HI_ID_VPSS, HI_ID_VOU>(it->second, 3, 12, it->first);
		}
		else
		{
			MPPSystem::UnBind<HI_ID_VPSS, HI_ID_VOU>(g_CurMainScene, 4, 12, it->first);
		}

		switch (it->second)
		{
		case TEA_FEATURE:
			vpss_tea_fea.StopUserChannal(3);
			break;
		case STU_FEATURE:
			vpss_stu_fea.StopUserChannal(3);
			break;
		case TEA_FULL_VIEW:
			vpss_tea_full.StopUserChannal(3);
			break;
		case STU_FULL_VIEW:
			vpss_stu_full.StopUserChannal(3);
			break;
		case BLACK_BOARD_FEATURE:
			vpss_black_board.StopUserChannal(3);
			break;
		case PC_CAPTURE:
			vpss_pc.StopUserChannal(3);
			break;
		default:
			break;
		}
	}

	vo_main.StopAllChn();
}

static int StartDisplayScreen()
{
	int ret;

	ret = vo_disp.Initialize({0, VO_INTF_VGA | VO_INTF_HDMI, Config::Instance()->system_.disp_vo_intf_sync});
	if (ret != KSuccess)
		return ret;

	for (auto it = Config::Instance()->system_.chns.begin(); it != Config::Instance()->system_.chns.end(); it++)
	{
		ret = vo_disp.StartChannel(it->chn, it->rect, 0);
		if (ret != KSuccess)
			return ret;
	}

	for (auto it = Config::Instance()->system_.mapping.begin(); it != Config::Instance()->system_.mapping.end(); it++)
	{
		if (it->second == MAIN)
		{
			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(6, 2, 0, it->first);
			if (ret != KSuccess)
				return ret;
		}
		else
		{
			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(it->second, 2, 0, it->first);
			if (ret != KSuccess)
				return ret;
		}
	}
	return KSuccess;
}

static void CloseDisplayScreen()
{
	for (auto it = Config::Instance()->system_.mapping.begin(); it != Config::Instance()->system_.mapping.end(); it++)
	{
		if (it->second == MAIN)
		{
			MPPSystem::UnBind<HI_ID_VPSS, HI_ID_VOU>(6, 2, 0, it->first);
		}
		else
		{
			MPPSystem::UnBind<HI_ID_VPSS, HI_ID_VOU>(it->second, 2, 0, it->first);
		}
	}
	vo_disp.StopAllChn();
	vo_disp.Close();
}

static int StartVideoEncode()
{
	int ret;
	if (Config::Instance()->IsResourceMode())
	{
		ret = vpss_tea_fea.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.res_width, (HI_U32)Config::Instance()->video_.res_height});
		if (ret != KSuccess)
			return ret;
		ret = vpss_stu_fea.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.res_width, (HI_U32)Config::Instance()->video_.res_height});
		if (ret != KSuccess)
			return ret;
		ret = vpss_tea_full.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.res_width, (HI_U32)Config::Instance()->video_.res_height});
		if (ret != KSuccess)
			return ret;
		ret = vpss_stu_full.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.res_width, (HI_U32)Config::Instance()->video_.res_height});
		if (ret != KSuccess)
			return ret;
		ret = vpss_black_board.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.res_width, (HI_U32)Config::Instance()->video_.res_height});
		if (ret != KSuccess)
			return ret;
		ret = vpss_pc.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.res_width, (HI_U32)Config::Instance()->video_.res_height});
		if (ret != KSuccess)
			return ret;
		ret = vpss_main.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.res_width, (HI_U32)Config::Instance()->video_.res_height});
		if (ret != KSuccess)
			return ret;

		ret = venc_tea_fea.Initialize({0, 0, Config::Instance()->video_.res_width, Config::Instance()->video_.res_height, 25, 25, 0, Config::Instance()->video_.res_bitrate, VENC_RC_MODE_H264CBR, false});
		if (ret != KSuccess)
			return ret;
		ret = venc_stu_fea.Initialize({1, 1, Config::Instance()->video_.res_width, Config::Instance()->video_.res_height, 25, 25, 0, Config::Instance()->video_.res_bitrate, VENC_RC_MODE_H264CBR, false});
		if (ret != KSuccess)
			return ret;
		ret = venc_tea_full.Initialize({2, 2, Config::Instance()->video_.res_width, Config::Instance()->video_.res_height, 25, 25, 0, Config::Instance()->video_.res_bitrate, VENC_RC_MODE_H264CBR, true});
		if (ret != KSuccess)
			return ret;
		ret = venc_stu_full.Initialize({3, 3, Config::Instance()->video_.res_width, Config::Instance()->video_.res_height, 25, 25, 0, Config::Instance()->video_.res_bitrate, VENC_RC_MODE_H264CBR, true});
		if (ret != KSuccess)
			return ret;
		ret = venc_black_board.Initialize({4, 4, Config::Instance()->video_.res_width, Config::Instance()->video_.res_height, 25, 25, 0, Config::Instance()->video_.res_bitrate, VENC_RC_MODE_H264CBR, true});
		if (ret != KSuccess)
			return ret;
		ret = venc_pc.Initialize({5, 5, Config::Instance()->video_.res_width, Config::Instance()->video_.res_height, 25, 25, 0, Config::Instance()->video_.res_bitrate, VENC_RC_MODE_H264CBR, true});
		if (ret != KSuccess)
			return ret;
		ret = venc_main.Initialize({6, 6, Config::Instance()->video_.res_width, Config::Instance()->video_.res_height, 25, 25, 0, Config::Instance()->video_.res_bitrate, VENC_RC_MODE_H264CBR, false});
		if (ret != KSuccess)
			return ret;

		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(0, 1, 0, 0);
		if (ret != KSuccess)
			return ret;
		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(1, 1, 1, 0);
		if (ret != KSuccess)
			return ret;
		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(2, 1, 2, 0);
		if (ret != KSuccess)
			return ret;
		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(3, 1, 3, 0);
		if (ret != KSuccess)
			return ret;
		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(4, 1, 4, 0);
		if (ret != KSuccess)
			return ret;
		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(5, 1, 5, 0);
		if (ret != KSuccess)
			return ret;
		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(6, 1, 6, 0);
		if (ret != KSuccess)
			return ret;
	}
	else
	{
		ret = vpss_tea_fea.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.normal_live_width, (HI_U32)Config::Instance()->video_.normal_live_height});
		if (ret != KSuccess)
			return ret;
		ret = vpss_stu_fea.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.normal_live_width, (HI_U32)Config::Instance()->video_.normal_live_height});
		if (ret != KSuccess)
			return ret;
		ret = vpss_tea_full.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.normal_live_width, (HI_U32)Config::Instance()->video_.normal_live_height});
		if (ret != KSuccess)
			return ret;
		ret = vpss_stu_full.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.normal_live_width, (HI_U32)Config::Instance()->video_.normal_live_height});
		if (ret != KSuccess)
			return ret;
		ret = vpss_black_board.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.normal_live_width, (HI_U32)Config::Instance()->video_.normal_live_height});
		if (ret != KSuccess)
			return ret;
		ret = vpss_pc.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.normal_live_width, (HI_U32)Config::Instance()->video_.normal_live_height});
		if (ret != KSuccess)
			return ret;
		ret = vpss_main.StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.normal_record_width, (HI_U32)Config::Instance()->video_.normal_record_height});
		if (ret != KSuccess)
			return ret;
		ret = vpss_main.StartUserChannel(3, {0, 0, (HI_U32)Config::Instance()->video_.normal_live_width, (HI_U32)Config::Instance()->video_.normal_live_height});
		if (ret != KSuccess)
			return ret;

		ret = venc_tea_fea.Initialize({0, 0, Config::Instance()->video_.normal_live_width, Config::Instance()->video_.normal_live_height, 25, 25, 0, Config::Instance()->video_.normal_live_bitrate, VENC_RC_MODE_H264CBR, false});
		if (ret != KSuccess)
			return ret;
		ret = venc_stu_fea.Initialize({1, 1, Config::Instance()->video_.normal_live_width, Config::Instance()->video_.normal_live_height, 25, 25, 0, Config::Instance()->video_.normal_live_bitrate, VENC_RC_MODE_H264CBR, false});
		if (ret != KSuccess)
			return ret;
		ret = venc_tea_full.Initialize({2, 2, Config::Instance()->video_.normal_live_width, Config::Instance()->video_.normal_live_height, 25, 25, 0, Config::Instance()->video_.normal_live_bitrate, VENC_RC_MODE_H264CBR, true});
		if (ret != KSuccess)
			return ret;
		ret = venc_stu_full.Initialize({3, 3, Config::Instance()->video_.normal_live_width, Config::Instance()->video_.normal_live_height, 25, 25, 0, Config::Instance()->video_.normal_live_bitrate, VENC_RC_MODE_H264CBR, true});
		if (ret != KSuccess)
			return ret;
		ret = venc_black_board.Initialize({4, 4, Config::Instance()->video_.normal_live_width, Config::Instance()->video_.normal_live_height, 25, 25, 0, Config::Instance()->video_.normal_live_bitrate, VENC_RC_MODE_H264CBR, true});
		if (ret != KSuccess)
			return ret;
		ret = venc_pc.Initialize({5, 5, Config::Instance()->video_.normal_live_width, Config::Instance()->video_.normal_live_height, 25, 25, 0, Config::Instance()->video_.normal_live_bitrate, VENC_RC_MODE_H264CBR, true});
		if (ret != KSuccess)
			return ret;
		ret = venc_main.Initialize({6, 6, Config::Instance()->video_.normal_live_width, Config::Instance()->video_.normal_live_height, 25, 25, 0, Config::Instance()->video_.normal_live_bitrate, VENC_RC_MODE_H264CBR, false});
		if (ret != KSuccess)
			return ret;
		ret = venc_main2.Initialize({7, 7, Config::Instance()->video_.normal_live_width, Config::Instance()->video_.normal_live_height, 25, 25, 0, Config::Instance()->video_.normal_live_bitrate, VENC_RC_MODE_H264CBR, false});
		if (ret != KSuccess)
			return ret;

		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(0, 1, 0, 0);
		if (ret != KSuccess)
			return ret;
		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(1, 1, 1, 0);
		if (ret != KSuccess)
			return ret;
		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(2, 1, 2, 0);
		if (ret != KSuccess)
			return ret;
		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(3, 1, 3, 0);
		if (ret != KSuccess)
			return ret;
		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(4, 1, 4, 0);
		if (ret != KSuccess)
			return ret;
		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(5, 1, 5, 0);
		if (ret != KSuccess)
			return ret;
		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(6, 1, 6, 0);
		if (ret != KSuccess)
			return ret;
		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(6, 3, 7, 0);
		if (ret != KSuccess)
			return ret;
	}
	return KSuccess;
}

static void CloseVideoEncode()
{
	if (Config::Instance()->IsResourceMode())
	{
		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(6, 1, 6, 0);
		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(5, 1, 5, 0);
		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(4, 1, 4, 0);
		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(3, 1, 3, 0);
		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(2, 1, 2, 0);
		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(1, 1, 1, 0);
		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(0, 1, 0, 0);

		venc_main.Close();
		venc_pc.Close();
		venc_black_board.Close();
		venc_stu_full.Close();
		venc_tea_full.Close();
		venc_stu_fea.Close();
		venc_tea_fea.Close();

		vpss_main.StopUserChannal(1);
		vpss_pc.StopUserChannal(1);
		vpss_black_board.StopUserChannal(1);
		vpss_stu_full.StopUserChannal(1);
		vpss_tea_full.StopUserChannal(1);
		vpss_stu_fea.StopUserChannal(1);
		vpss_tea_fea.StopUserChannal(1);
	}
	else
	{
		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(6, 3, 7, 0);
		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(6, 1, 6, 0);
		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(5, 1, 5, 0);
		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(4, 1, 4, 0);
		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(3, 1, 3, 0);
		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(2, 1, 2, 0);
		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(1, 1, 1, 0);
		MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(0, 1, 0, 0);

		venc_main2.Close();
		venc_main.Close();
		venc_pc.Close();
		venc_black_board.Close();
		venc_stu_full.Close();
		venc_tea_full.Close();
		venc_stu_fea.Close();
		venc_tea_fea.Close();

		vpss_main.StopUserChannal(3);
		vpss_main.StopUserChannal(1);
		vpss_pc.StopUserChannal(1);
		vpss_black_board.StopUserChannal(1);
		vpss_stu_full.StopUserChannal(1);
		vpss_tea_full.StopUserChannal(1);
		vpss_stu_fea.StopUserChannal(1);
		vpss_tea_fea.StopUserChannal(1);
	}
}

static int StartRecord(const std::vector<std::pair<RS_SCENE, mp4::Params>> &records)
{
	int ret;
	for (const std::pair<RS_SCENE, mp4::Params> &rec : records)
	{
		switch (rec.first)
		{
		case TEA_FEATURE:
		{
			ret = rec_tea_fea.Initialize(rec.second);
			if (ret != KSuccess)
				return ret;
			aenc_main.AddAudioSink(&rec_tea_fea);
			venc_tea_fea.AddVideoSink(&rec_tea_fea);
			break;
		}
		case STU_FEATURE:
		{
			ret = rec_stu_fea.Initialize(rec.second);
			if (ret != KSuccess)
				return ret;
			aenc_main.AddAudioSink(&rec_stu_fea);
			venc_stu_fea.AddVideoSink(&rec_stu_fea);
			break;
		}

		case TEA_FULL_VIEW:
		{
			ret = rec_tea_full.Initialize(rec.second);
			if (ret != KSuccess)
				return ret;
			aenc_main.AddAudioSink(&rec_tea_full);
			venc_tea_full.AddVideoSink(&rec_tea_full);
			break;
		}
		case STU_FULL_VIEW:
		{
			ret = rec_stu_full.Initialize(rec.second);
			if (ret != KSuccess)
				return ret;
			aenc_main.AddAudioSink(&rec_stu_full);
			venc_stu_full.AddVideoSink(&rec_stu_full);
			break;
		}
		case BLACK_BOARD_FEATURE:
		{
			ret = rec_black_board.Initialize(rec.second);
			if (ret != KSuccess)
				return ret;
			aenc_main.AddAudioSink(&rec_black_board);
			venc_black_board.AddVideoSink(&rec_black_board);
			break;
		}
		case PC_CAPTURE:
		{
			ret = rec_pc.Initialize(rec.second);
			if (ret != KSuccess)
				return ret;
			aenc_main.AddAudioSink(&rec_pc);
			venc_pc.AddVideoSink(&rec_pc);
			break;
		}
		case MAIN:
		{
			ret = rec_main.Initialize(rec.second);
			if (ret != KSuccess)
				return ret;
			aenc_main.AddAudioSink(&rec_main);
			venc_main.AddVideoSink(&rec_main);
			break;
		}
		default:
			break;
		}
	}

	return KSuccess;
}

static void CloseRecord()
{
	aenc_main.RemoveAudioSink(&rec_tea_fea);
	venc_tea_fea.RemoveVideoSink(&rec_tea_fea);
	aenc_main.RemoveAudioSink(&rec_stu_fea);
	venc_stu_fea.RemoveVideoSink(&rec_stu_fea);
	aenc_main.RemoveAudioSink(&rec_tea_full);
	venc_tea_full.RemoveVideoSink(&rec_tea_full);
	aenc_main.RemoveAudioSink(&rec_stu_full);
	venc_stu_full.RemoveVideoSink(&rec_stu_full);
	aenc_main.RemoveAudioSink(&rec_black_board);
	venc_black_board.RemoveVideoSink(&rec_black_board);
	aenc_main.RemoveAudioSink(&rec_pc);
	venc_pc.RemoveVideoSink(&rec_pc);
	aenc_main.RemoveAudioSink(&rec_main);
	venc_main.RemoveVideoSink(&rec_main);

	rec_tea_fea.Close();
	rec_stu_fea.Close();
	rec_tea_full.Close();
	rec_stu_full.Close();
	rec_black_board.Close();
	rec_pc.Close();
	rec_main.Close();
}

static int StartLive(const std::vector<std::pair<RS_SCENE, rtmp::Params>> &lives)
{
	int ret;
	for (const std::pair<RS_SCENE, rtmp::Params> &live : lives)
	{
		switch (live.first)
		{
		case TEA_FEATURE:
		{
			ret = live_tea_fea.Initialize(live.second);
			if (ret != KSuccess)
				return ret;
			venc_tea_fea.AddVideoSink(&live_tea_fea);
			break;
		}
		case STU_FEATURE:
		{
			ret = live_stu_fea.Initialize(live.second);
			if (ret != KSuccess)
				return ret;
			venc_stu_fea.AddVideoSink(&live_stu_fea);
			break;
		}

		case TEA_FULL_VIEW:
		{
			ret = live_tea_full.Initialize(live.second);
			if (ret != KSuccess)
				return ret;
			venc_tea_full.AddVideoSink(&live_tea_full);
			break;
		}
		case STU_FULL_VIEW:
		{
			ret = live_stu_full.Initialize(live.second);
			if (ret != KSuccess)
				return ret;
			venc_stu_full.AddVideoSink(&live_stu_full);
			break;
		}
		case BLACK_BOARD_FEATURE:
		{
			ret = live_black_board.Initialize(live.second);
			if (ret != KSuccess)
				return ret;
			venc_black_board.AddVideoSink(&live_black_board);
			break;
		}
		case PC_CAPTURE:
		{
			ret = live_pc.Initialize(live.second);
			if (ret != KSuccess)
				return ret;
			venc_pc.AddVideoSink(&live_pc);
			break;
		}
		case MAIN:
		{
			ret = live_main.Initialize(live.second);
			if (ret != KSuccess)
				return ret;
			aenc_main.AddAudioSink(&live_main);
			if (Config::Instance()->IsResourceMode())
			{
				venc_main.AddVideoSink(&live_main);
			}
			else
			{
				venc_main2.AddVideoSink(&live_main);
			}
			break;
		}
		case MAIN2:
		{
			ret = live_main2.Initialize(live.second);
			if (ret != KSuccess)
				return ret;
			aenc_main.AddAudioSink(&live_main2);
			if (Config::Instance()->IsResourceMode())
			{
				venc_main.AddVideoSink(&live_main2);
			}
			else
			{
				venc_main2.AddVideoSink(&live_main2);
			}
			break;
		}
		default:
			break;
		}
	}

	return KSuccess;
}

void CloseLive()
{
	venc_tea_fea.RemoveVideoSink(&rec_tea_fea);
	venc_stu_fea.RemoveVideoSink(&rec_stu_fea);
	venc_tea_full.RemoveVideoSink(&rec_tea_full);
	venc_stu_full.RemoveVideoSink(&rec_stu_full);
	venc_black_board.RemoveVideoSink(&rec_black_board);
	venc_pc.RemoveVideoSink(&rec_pc);
	aenc_main.RemoveAudioSink(&live_main);
	aenc_main.RemoveAudioSink(&live_main2);
	if (Config::Instance()->IsResourceMode())
	{
		venc_main.RemoveVideoSink(&live_main);
		venc_main.RemoveVideoSink(&live_main2);
	}
	else
	{
		venc_main2.RemoveVideoSink(&live_main);
		venc_main2.RemoveVideoSink(&live_main2);
	}
}

static void StartRecordHandler(evhttp_request *req, void *arg)
{
	int ret;

	std::string str = HttpServer::GetRequestData(req);

	Json::Value root;
	if (JsonUtils::toJson(str, root) != KSuccess)
	{
		HttpServer::MakeResponse(req, HTTP_SERVUNAVAIL, "format error", "{\"errMsg\":\"parse json root failed\"}");
		return;
	}

	if (!RecordReq::IsOk(root))
	{
		HttpServer::MakeResponse(req, HTTP_SERVUNAVAIL, "format error", "{\"errMsg\":\"check json format failed\"}");
		return;
	}

	RecordReq record_req;
	record_req = root;

	CloseRecord();
	ret = StartRecord(record_req.recs);
	if (ret != KSuccess)
	{
		HttpServer::MakeResponse(req, HTTP_INTERNAL, "system error", "{\"errMsg\":\"start record failed\"}");
		return;
	}

	HttpServer::MakeResponse(req, HTTP_OK, "ok", "{\"errMsg\":\"success\"}");
}

static void StopRecordHandler(evhttp_request *req, void *arg)
{
	CloseRecord();
	HttpServer::MakeResponse(req, HTTP_OK, "ok", "{\"errMsg\":\"success\"}");
}

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

	ret = ai_main.Initialize({4, 0});
	CHECK_ERROR(ret);

	ret = aenc_main.Initialize();
	CHECK_ERROR(ret);

	ret = ao_main.Initialize({4, 0});
	CHECK_ERROR(ret);

	ret = PCIVComm::Instance()->Initialize();
	CHECK_ERROR(ret);
	ret = PCIVTrans::Instance()->Initialize(PCIVComm::Instance());
	CHECK_ERROR(ret);

	ret = vdec_tea_full.Initialize({0, RS_MAX_WIDTH, RS_MAX_HEIGHT});
	CHECK_ERROR(ret);
	ret = vdec_stu_full.Initialize({1, RS_MAX_WIDTH, RS_MAX_HEIGHT});
	CHECK_ERROR(ret);
	ret = vdec_black_board.Initialize({2, RS_MAX_WIDTH, RS_MAX_WIDTH});
	CHECK_ERROR(ret);
	ret = vdec_pc.Initialize({3, RS_MAX_WIDTH, RS_MAX_HEIGHT});
	CHECK_ERROR(ret);

	ret = vpss_tea_fea_2vo.Initialize({10});
	CHECK_ERROR(ret);
	ret = vpss_stu_fea_2vo.Initialize({11});
	CHECK_ERROR(ret);
	ret = vpss_tea_fea.Initialize({0});
	CHECK_ERROR(ret);
	ret = vpss_stu_fea.Initialize({1});
	CHECK_ERROR(ret);
	ret = vpss_tea_full.Initialize({2});
	CHECK_ERROR(ret);
	ret = vpss_stu_full.Initialize({3});
	CHECK_ERROR(ret);
	ret = vpss_black_board.Initialize({4});
	CHECK_ERROR(ret);
	ret = vpss_pc.Initialize({5});
	CHECK_ERROR(ret);
	ret = vpss_main.Initialize({6});
	CHECK_ERROR(ret);

	ret = vo_tea_fea.Initialize({10, 0, VO_OUTPUT_1080P25});
	CHECK_ERROR(ret);
	ret = vo_stu_fea.Initialize({11, 0, VO_OUTPUT_1080P25});
	CHECK_ERROR(ret);
	ret = vo_main.Initialize({12, 0, VO_OUTPUT_1080P25});
	CHECK_ERROR(ret);

	ret = SigDetect::Instance()->Initialize(PCIVComm::Instance(), Config::Instance()->system_.pc_capture_mode);
	CHECK_ERROR(ret);

	VIHelper vi_tea_fea(4, 8, &vo_tea_fea);
	VIHelper vi_stu_fea(2, 4, &vo_stu_fea);
	ret = vo_tea_fea.StartChannel(0, {0, 0, RS_MAX_WIDTH, RS_MAX_HEIGHT}, 0);
	CHECK_ERROR(ret);
	ret = vo_stu_fea.StartChannel(0, {0, 0, RS_MAX_WIDTH, RS_MAX_HEIGHT}, 0);
	CHECK_ERROR(ret);
	ai_main.AddAudioSink(&aenc_main);

	SigDetect::Instance()->AddVIFmtListener(&vi_tea_fea);
	SigDetect::Instance()->AddVIFmtListener(&vi_stu_fea);
	PCIVTrans::Instance()->AddVideoSink(&vdec_tea_full);
	PCIVTrans::Instance()->AddVideoSink(&vdec_stu_full);
	PCIVTrans::Instance()->AddVideoSink(&vdec_black_board);
	PCIVTrans::Instance()->AddVideoSink(&vdec_pc);

	ret = MPPSystem::Bind<HI_ID_VIU, HI_ID_VPSS>(0, 8, 10, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VIU, HI_ID_VPSS>(0, 4, 11, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(10, 4, 10, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(11, 4, 11, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VOU, HI_ID_VPSS>(10, 0, 0, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VOU, HI_ID_VPSS>(11, 0, 1, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 0, 2, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 1, 3, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 2, 4, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 3, 5, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_AI, HI_ID_AO>(4, 0, 4, 0);
	CHECK_ERROR(ret);

	ret = StartVideoEncode();
	CHECK_ERROR(ret);

	ret = StartDisplayScreen();
	CHECK_ERROR(ret);

	ret = StartMainScreen();
	CHECK_ERROR(ret);

	ret = StartLive({{MAIN, {"rtmp://127.0.0.1/live/main", true}},
					 {TEA_FEATURE, {"rtmp://127.0.0.1/live/tea_fea", false}},
					 {STU_FEATURE, {"rtmp://127.0.0.1/live/stu_fea", false}},
					 {TEA_FULL_VIEW, {"rtmp://127.0.0.1/live/tea_full", false}},
					 {STU_FULL_VIEW, {"rtmp://127.0.0.1/live/stu_full", false}},
					 {BLACK_BOARD_FEATURE, {"rtmp://127.0.0.1/live/black_board", false}},
					 {PC_CAPTURE, {"rtmp://127.0.0.1/live/pc", false}},
					 {MAIN2, {"rtmp://127.0.0.1/live/main2", true}}});
	CHECK_ERROR(ret)

	// ret = StartRecord({{MAIN, {1280, 720, 25, 44100, "/nand/main.mp4", 0, false}},
	// 				   {TEA_FEATURE, {1280, 720, 25, 44100, "/nand/tea_fea.mp4", 0, false}},
	// 				   {STU_FEATURE, {1280, 720, 25, 44100, "/nand/stu_fea.mp4", 0, false}},
	// 				   {TEA_FULL_VIEW, {1280, 720, 25, 44100, "/nand/tea_full.mp4", 0, false}},
	// 				   {STU_FULL_VIEW, {1280, 720, 25, 44100, "/nand/stu_full", 0, false}},
	// 				   {BLACK_BOARD_FEATURE, {1280, 720, 25, 44100, "/nand/black_board.mp4", 0, false}},
	// 				   {PC_CAPTURE, {1280, 720, 25, 44100, "/nand/pc.mp4", 0, false}}});
	// CHECK_ERROR(ret)

	HttpServer http_server;
	http_server.Initialize("0.0.0.0", 8081);
	http_server.RegisterURI("/start_record", StartRecordHandler, nullptr);
	http_server.RegisterURI("/stop_record", StopRecordHandler, nullptr);

	while (g_Run)
		http_server.Dispatch();

	CloseRecord();
	CloseLive();
	CloseMainScreen();
	CloseDisplayScreen();
	CloseVideoEncode();

	SigDetect::Instance()->Close();
	vo_main.Close();
	vo_stu_fea.Close();
	vo_tea_fea.Close();
	vpss_main.Close();
	vpss_pc.Close();
	vpss_black_board.Close();
	vpss_stu_full.Close();
	vpss_tea_full.Close();
	vpss_stu_fea.Close();
	vpss_tea_fea.Close();
	vpss_stu_fea_2vo.Close();
	vpss_tea_fea_2vo.Close();
	vdec_pc.Close();
	vdec_black_board.Close();
	vdec_stu_full.Close();
	vdec_tea_full.Close();
	PCIVTrans::Instance()->Close();
	PCIVComm::Instance()->Close();
	ao_main.Close();
	aenc_main.Close();
	ai_main.Close();
	MPPSystem::Instance()->Close();

	return 0;
}
