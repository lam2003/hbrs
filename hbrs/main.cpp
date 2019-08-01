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
#include "record/mp4_record.h"
#include "live/rtmp_live.h"

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
		g_Run = false;
	}
}

int32_t main(int32_t argc, char **argv)
{
	int ret;

	AudioInput ai;
	AudioEncode aenc;
	AudioOutput ao;
	VideoProcess vpss_tea_fea;
	VideoProcess vpss_stu_fea;
	VideoProcess vpss_tea_full;
	VideoProcess vpss_stu_full;
	VideoProcess vpss_black_board;
	VideoProcess vpss_pc;
	VideoProcess vpss_main;
	VideoProcess vpss_tea_fea_2vo;
	VideoProcess vpss_stu_fea_2vo;
	VideoEncode venc_tea_fea;
	VideoEncode venc_stu_fea;
	VideoEncode venc_tea_full;
	VideoEncode venc_stu_full;
	VideoEncode venc_black_board;
	VideoEncode venc_pc;
	VideoEncode venc_main;
	VideoEncode venc_main2;
	VideoDecode vdec_tea_full;
	VideoDecode vdec_stu_full;
	VideoDecode vdec_black_board;
	VideoDecode vdec_pc;
	VideoOutput vo_tea_fea;
	VideoOutput vo_stu_fea;
	VideoOutput vo_main;
	VideoOutput vo_disp;

	ConfigLogger();

	signal(SIGINT, SignalHandler);

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

	//#####################################################
	//初始化MPP
	//#####################################################
	ret = MPPSystem::Instance()->Initialize();
	CHECK_ERROR(ret);
	//#####################################################
	//初始化音频输入
	//#####################################################
	ret = ai.Initialize({4, 0});
	CHECK_ERROR(ret);
	//#####################################################
	//初始化音频编码
	//#####################################################
	ret = aenc.Initialize();
	CHECK_ERROR(ret);
	ai.AddAudioSink(&aenc);
	//#####################################################
	//初始化音频输出
	//#####################################################
	ret = ao.Initialize({4, 0});
	CHECK_ERROR(ret);
	MPPSystem::Bind<HI_ID_AI, HI_ID_AO>(4, 0, 4, 0);
	//#####################################################
	//初始化从片数据的四路解码器,pciv->vdec
	//#####################################################
	ret = vdec_tea_full.Initialize({0, RS_MAX_WIDTH, RS_MAX_HEIGHT});
	CHECK_ERROR(ret);
	ret = vdec_stu_full.Initialize({1, RS_MAX_WIDTH, RS_MAX_HEIGHT});
	CHECK_ERROR(ret);
	ret = vdec_black_board.Initialize({2, RS_MAX_WIDTH, RS_MAX_WIDTH});
	CHECK_ERROR(ret);
	ret = vdec_pc.Initialize({3, RS_MAX_WIDTH, RS_MAX_HEIGHT});
	CHECK_ERROR(ret);
	//#####################################################
	//初始化从片上的四路vi,vi->vpss->vir_vo->venc->pciv
	//#####################################################
	ret = PCIVComm::Instance()->Initialize();
	CHECK_ERROR(ret);
	ret = PCIVTrans::Instance()->Initialize(PCIVComm::Instance());
	CHECK_ERROR(ret);
	PCIVTrans::Instance()->AddVideoSink(&vdec_tea_full);
	PCIVTrans::Instance()->AddVideoSink(&vdec_stu_full);
	PCIVTrans::Instance()->AddVideoSink(&vdec_black_board);
	PCIVTrans::Instance()->AddVideoSink(&vdec_pc);
	//#####################################################
	//初始化主片上的两路vi,vi->vpss->vir_vo
	//#####################################################
	ret = vpss_tea_fea_2vo.Initialize({10});
	CHECK_ERROR(ret);
	ret = vpss_stu_fea_2vo.Initialize({11});
	CHECK_ERROR(ret);
	ret = vo_tea_fea.Initialize({10, 0, VO_OUTPUT_1080P25});
	CHECK_ERROR(ret);
	ret = vo_tea_fea.StartChannel(0, {0, 0, RS_MAX_WIDTH, RS_MAX_HEIGHT}, 0);
	CHECK_ERROR(ret);
	ret = vo_stu_fea.Initialize({11, 0, VO_OUTPUT_1080P25});
	CHECK_ERROR(ret);
	ret = vo_stu_fea.StartChannel(0, {0, 0, RS_MAX_WIDTH, RS_MAX_HEIGHT}, 0);
	CHECK_ERROR(ret);
	VIHelper vi_tea_fea(4, 8, &vo_tea_fea);
	VIHelper vi_stu_fea(2, 4, &vo_stu_fea);
	ret = MPPSystem::Bind<HI_ID_VIU, HI_ID_VPSS>(0, 8, 10, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VIU, HI_ID_VPSS>(0, 4, 11, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(10, 4, 10, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(11, 4, 11, 0);
	CHECK_ERROR(ret);
	//#####################################################
	//初始化信号检测器
	//#####################################################
	ret = SigDetect::Instance()->Initialize(PCIVComm::Instance(), Config::Instance()->system_.pc_capture_mode);
	CHECK_ERROR(ret);
	SigDetect::Instance()->AddVIFmtListener(&vi_tea_fea);
	SigDetect::Instance()->AddVIFmtListener(&vi_stu_fea);
	//#####################################################
	//初始化六路原始视频使用的vpss vir_vo->vpss/vdec->vpss
	//#####################################################
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
	//#####################################################
	//初始化主画面,由原始输入视频合成,vir_vo->vpss
	//#####################################################
	ret = vo_main.Initialize({12, 0, VO_OUTPUT_1080P25});
	CHECK_ERROR(ret);
	ret = vo_main.StopAllChn();
	CHECK_ERROR(ret);

	std::map<int, std::pair<RECT_S, int>> scene_pos = VideoOutput::GetScenePos(Config::Instance()->scene_.mode);
	for (auto it = Config::Instance()->scene_.mapping.begin(); it != Config::Instance()->scene_.mapping.end(); it++)
	{
		if (scene_pos.count(it->first) == 0)
			continue;

		ret = vo_main.StartChannel(it->first, scene_pos[it->first].first, scene_pos[it->first].second);
		CHECK_ERROR(ret);

		switch (it->second)
		{
		case TEA_FEATURE:
			vpss_tea_fea.StartUserChannel(3, scene_pos[it->first].first);
			CHECK_ERROR(ret);
			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(it->second, 3, 12, it->first);
			CHECK_ERROR(ret);
			break;
		case STU_FEATURE:
			vpss_stu_fea.StartUserChannel(3, scene_pos[it->first].first);
			CHECK_ERROR(ret);
			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(it->second, 3, 12, it->first);
			CHECK_ERROR(ret);
			break;
		case TEA_FULL_VIEW:
			vpss_tea_full.StartUserChannel(3, scene_pos[it->first].first);
			CHECK_ERROR(ret);
			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(it->second, 3, 12, it->first);
			CHECK_ERROR(ret);
			break;
		case STU_FULL_VIEW:
			vpss_stu_full.StartUserChannel(3, scene_pos[it->first].first);
			CHECK_ERROR(ret);
			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(it->second, 3, 12, it->first);
			CHECK_ERROR(ret);
			break;
		case BLACK_BOARD_FEATURE:
			vpss_black_board.StartUserChannel(3, scene_pos[it->first].first);
			CHECK_ERROR(ret);
			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(it->second, 3, 12, it->first);
			CHECK_ERROR(ret);
			break;
		case PC_CAPTURE:
			vpss_pc.StartUserChannel(3, scene_pos[it->first].first);
			CHECK_ERROR(ret);
			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(it->second, 3, 12, it->first);
			CHECK_ERROR(ret);
			break;
		case MAIN:
			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(g_CurMainScene, 4, 12, it->first);
			CHECK_ERROR(ret);
			break;
		default:
			break;
		}
	}
	ret = vpss_main.Initialize({6});
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VOU, HI_ID_VPSS>(12, 0, 6, 0);
	CHECK_ERROR(ret);
	//#####################################################
	//初始化屏幕显示 vpss->phy_vo
	//#####################################################
	ret = vo_disp.Initialize({0, VO_INTF_VGA | VO_INTF_HDMI, Config::Instance()->system_.disp_vo_intf_sync});
	CHECK_ERROR(ret);
	for (auto it = Config::Instance()->system_.chns.begin(); it != Config::Instance()->system_.chns.end(); it++)
	{
		ret = vo_disp.StartChannel(it->chn, it->rect, 0);
		CHECK_ERROR(ret);
	}
	for (auto it = Config::Instance()->system_.mapping.begin(); it != Config::Instance()->system_.mapping.end(); it++)
	{
		if (it->second == MAIN)
		{
			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(6, 2, 0, it->first);
			CHECK_ERROR(ret);
		}
		else
		{
			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(it->second, 2, 0, it->first);
			CHECK_ERROR(ret);
		}
	}
	//#####################################################
	//初始化视频编码器,具有录制/直播双码流,vpss->venc
	//#####################################################
	int width;
	int height;
	int bitrate;

	if (Config::Instance()->IsResourceMode())
	{
		width = Config::Instance()->video_.res_width;
		height = Config::Instance()->video_.res_height;
		bitrate = Config::Instance()->video_.res_bitrate;
	}
	else
	{
		width = Config::Instance()->video_.normal_live_width;
		height = Config::Instance()->video_.normal_live_height;
		bitrate = Config::Instance()->video_.normal_live_bitrate;
	}

	ret = vpss_tea_fea.StartUserChannel(1, {0, 0, width, height});
	CHECK_ERROR(ret);
	ret = vpss_stu_fea.StartUserChannel(1, {0, 0, width, height});
	CHECK_ERROR(ret);
	ret = vpss_tea_full.StartUserChannel(1, {0, 0, width, height});
	CHECK_ERROR(ret);
	ret = vpss_stu_full.StartUserChannel(1, {0, 0, width, height});
	CHECK_ERROR(ret);
	ret = vpss_black_board.StartUserChannel(1, {0, 0, width, height});
	CHECK_ERROR(ret);
	ret = vpss_pc.StartUserChannel(1, {0, 0, width, height});
	CHECK_ERROR(ret);

	if (Config::Instance()->IsResourceMode())
	{
		ret = vpss_main.StartUserChannel(1, {0, 0, Config::Instance()->video_.res_width, Config::Instance()->video_.res_height});
		CHECK_ERROR(ret);
	}
	else
	{
		ret = vpss_main.StartUserChannel(1, {0, 0, Config::Instance()->video_.normal_record_width, Config::Instance()->video_.normal_record_height});
		CHECK_ERROR(ret);
		ret = vpss_main.StartUserChannel(3, {0, 0, Config::Instance()->video_.normal_live_width, Config::Instance()->video_.normal_live_height});
		CHECK_ERROR(ret);
	}

	ret = venc_tea_fea.Initialize({0, 0, width, height, RS_FRAME_RATE, RS_FRAME_RATE, 0, bitrate, VENC_RC_MODE_H264CBR});
	CHECK_ERROR(ret);
	ret = venc_stu_fea.Initialize({1, 1, width, height, RS_FRAME_RATE, RS_FRAME_RATE, 0, bitrate, VENC_RC_MODE_H264CBR});
	CHECK_ERROR(ret);
	ret = venc_tea_full.Initialize({2, 2, width, height, RS_FRAME_RATE, RS_FRAME_RATE, 0, bitrate, VENC_RC_MODE_H264CBR, true});
	CHECK_ERROR(ret);
	ret = venc_stu_full.Initialize({3, 3, width, height, RS_FRAME_RATE, RS_FRAME_RATE, 0, bitrate, VENC_RC_MODE_H264CBR, true});
	CHECK_ERROR(ret);
	ret = venc_black_board.Initialize({4, 4, width, height, RS_FRAME_RATE, RS_FRAME_RATE, 0, bitrate, VENC_RC_MODE_H264CBR, true});
	CHECK_ERROR(ret);
	ret = venc_pc.Initialize({5, 5, width, height, RS_FRAME_RATE, RS_FRAME_RATE, 0, bitrate, VENC_RC_MODE_H264CBR, true});
	CHECK_ERROR(ret);
	if (Config::Instance()->IsResourceMode())
	{
		ret = venc_main.Initialize({6, 6, Config::Instance()->video_.res_width, Config::Instance()->video_.res_height, RS_FRAME_RATE, RS_FRAME_RATE, 0, Config::Instance()->video_.res_bitrate, VENC_RC_MODE_H264CBR});
		CHECK_ERROR(ret);
	}
	else
	{
		ret = venc_main.Initialize({6, 6, Config::Instance()->video_.normal_record_width, Config::Instance()->video_.normal_record_height, RS_FRAME_RATE, RS_FRAME_RATE, 0, Config::Instance()->video_.normal_record_bitrate, VENC_RC_MODE_H264CBR});
		CHECK_ERROR(ret);
		ret = venc_main2.Initialize({7, 7, Config::Instance()->video_.normal_live_width, Config::Instance()->video_.normal_live_height, RS_FRAME_RATE, RS_FRAME_RATE, 0, Config::Instance()->video_.normal_live_bitrate, VENC_RC_MODE_H264CBR});
		CHECK_ERROR(ret);
	}

	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(0, 1, 0, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(1, 1, 1, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(2, 1, 2, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(3, 1, 3, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(4, 1, 4, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(5, 1, 5, 0);
	CHECK_ERROR(ret);
	if (Config::Instance()->IsResourceMode())
	{
		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(6, 1, 6, 0);
		CHECK_ERROR(ret);
	}
	else
	{
		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(6, 1, 6, 0);
		CHECK_ERROR(ret);
		ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(6, 3, 7, 0);
		CHECK_ERROR(ret);
	}

	HttpServer http_server;
	http_server.Initialize("0.0.0.0", 8081);

	MP4Record record_tea_fea;
	MP4Record record_stu_fea;
	MP4Record record_tea_full;
	MP4Record record_stu_full;
	MP4Record record_black_board;
	MP4Record record_pc;
	MP4Record record_main;

	// record_tea_fea.Initialize({1280, 720, 25, 44100, "./record_tea_fea.mp4", 10, false});
	// record_stu_fea.Initialize({1280, 720, 25, 44100, "./record_stu_fea.mp4", 10, false});
	// record_tea_full.Initialize({1280, 720, 25, 44100, "./record_tea_full.mp4", 10, false});
	// record_stu_full.Initialize({1280, 720, 25, 44100, "/nand/record_stu_full.mp4", 10, false});
	// record_black_board.Initialize({1280, 720, 25, 44100, "/nand/record_black_board.mp4", 10, false});
	// record_pc.Initialize({1280, 720, 25, 44100, "/nand/record_pc.mp4", 10, false});
	record_main.Initialize({1280, 720, 25, 44100, "/nand/record_main.mp4", 10, false});

	// aenc.AddAudioSink(&record_tea_fea);
	// aenc.AddAudioSink(&record_stu_fea);
	// aenc.AddAudioSink(&record_tea_full);
	// aenc.AddAudioSink(&record_stu_full);
	// aenc.AddAudioSink(&record_black_board);
	// aenc.AddAudioSink(&record_pc);
	aenc.AddAudioSink(&record_main);

	// venc_tea_fea.AddVideoSink(&record_tea_fea);
	// venc_stu_fea.AddVideoSink(&record_stu_fea);
	// venc_tea_full.AddVideoSink(&record_tea_full);
	// venc_stu_full.AddVideoSink(&record_stu_full);
	// venc_black_board.AddVideoSink(&record_black_board);
	// venc_pc.AddVideoSink(&record_pc);
	venc_main.AddVideoSink(&record_main);

printf("##############1111\n");
	RTMPLive live_tea_fea;
	printf("##############2222\n");
	RTMPLive live_stu_fea;
		printf("##############3333\n");
	RTMPLive live_tea_full;
			printf("##############4444\n");
	RTMPLive live_stu_full;
		printf("##############5555\n");
	RTMPLive live_black_board;
			printf("##############6666\n");
	RTMPLive live_pc;
			printf("##############7777\n");
	RTMPLive live_main;
		printf("##############8888\n");
	live_tea_fea.Initialize({"rtmp://127.0.0.1/live/live_tea_fea"});
			printf("##############9999\n");
	live_stu_fea.Initialize({"rtmp://127.0.0.1/live/live_stu_fea"});
		printf("##############aaaa\n");
	live_tea_full.Initialize({"rtmp://127.0.0.1/live/live_tea_full"});
		printf("##############bbbb\n");
	live_stu_full.Initialize({"rtmp://127.0.0.1/live/live_stu_full"});
		printf("##############cccc\n");
	live_black_board.Initialize({"rtmp://127.0.0.1/live/live_black_board"});
			printf("##############dddd\n");
	live_pc.Initialize({"rtmp://127.0.0.1/live/live_pc"});
		printf("##############eeee\n");
	live_main.Initialize({"rtmp://127.0.0.1/live/live_main"});
		printf("##############ffff\n");

	aenc.AddAudioSink(&live_main);

	venc_tea_fea.AddVideoSink(&live_tea_fea);
	venc_stu_fea.AddVideoSink(&live_stu_fea);
	venc_tea_full.AddVideoSink(&live_tea_full);
	venc_stu_full.AddVideoSink(&live_stu_full);
	venc_black_board.AddVideoSink(&live_black_board);
	venc_pc.AddVideoSink(&live_pc);
	venc_main.AddVideoSink(&live_main);

	while (g_Run)
	{
		http_server.Dispatch();
	}

	aenc.RemoveAllAudioSink();
	venc_main.RemoveAllVideoSink();
	venc_pc.RemoveAllVideoSink();
	venc_tea_fea.RemoveAllVideoSink();
	venc_stu_fea.RemoveAllVideoSink();
	venc_tea_full.RemoveAllVideoSink();
	venc_stu_full.RemoveAllVideoSink();
	venc_black_board.RemoveAllVideoSink();

	// record_tea_fea.Close();
	// record_stu_fea.Close();
	// record_tea_full.Close();
	// record_stu_full.Close();
	// record_black_board.Close();
	// record_pc.Close();
	record_main.Close();

	live_tea_fea.Close();
	live_stu_fea.Close();
	live_tea_full.Close();
	live_stu_full.Close();
	live_black_board.Close();
	live_pc.Close();
	live_main.Close();

	return 0;
}
