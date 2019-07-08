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
#include "system/aenc.h"
#include "common/buffer.h"
#include "common/config.h"

using namespace rs;

static VIHelper vi_tea_fea(4, 8);
static VIHelper vi_stu_fea(2, 4);

static VideoProcess vpss_tea_fea;
static VideoProcess vpss_stu_fea;
static VideoProcess vpss_tea_full;
static VideoProcess vpss_stu_full;
static VideoProcess vpss_black_board;
static VideoProcess vpss_pc;
static VideoProcess vpss_main;
static VideoProcess vpss_tea_fea_2vo;
static VideoProcess vpss_stu_fea_2vo;

static VideoEncode venc_tea_fea_record;
static VideoEncode venc_stu_fea_record;
static VideoEncode venc_tea_full_record;
static VideoEncode venc_stu_full_record;
static VideoEncode venc_black_board_record;
static VideoEncode venc_pc_record;
static VideoEncode venc_main_record;
static VideoEncode venc_tea_fea_live;
static VideoEncode venc_stu_fea_live;
static VideoEncode venc_tea_full_live;
static VideoEncode venc_stu_full_live;
static VideoEncode venc_black_board_live;
static VideoEncode venc_pc_live;
static VideoEncode venc_main_live;

static VideoDecode vdec_tea_full;
static VideoDecode vdec_stu_full;
static VideoDecode vdec_black_board;
static VideoDecode vdec_pc;

static VideoOutput vo_tea_fea;
static VideoOutput vo_stu_fea;
static VideoOutput vo_main;
static VideoOutput vo_disp;

#define CHECK_ERROR(a) \
	if (KSuccess != a) \
	{                  \
		return a;      \
	}
#define CHECK_ERROR_EXT(a)                                                              \
	if (KSuccess != a)                                                                  \
	{                                                                                   \
		log_e("error:%s", make_error_code(static_cast<err_code>(a)).message().c_str()); \
		return a;                                                                       \
	}

static const char *g_Opts = "c:";
struct option g_LongOpts[] = {
	{"config", 1, NULL, 'c'},
	{0, 0, 0, 0}};

static RS_SCENE g_CurMainScene = TEA_FEATURE;

static bool g_Run = true;

static void SignalHandler(int signo)
{
	if (signo == SIGINT)
	{
		g_Run = false;
	}
}

static void ConfigLogger()
{
	setbuf(stdout, NULL);
	elog_init();
	elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);
	elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TIME | ELOG_FMT_DIR |
									 ELOG_FMT_LINE | ELOG_FMT_FUNC);
	elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TIME);
	elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TIME);
	elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_LVL | ELOG_FMT_TIME);
	elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_ALL & ~ELOG_FMT_FUNC);
	elog_set_text_color_enabled(true);
	elog_start();
}

static int SetSceneMode(VideoOutput &vo, int mode)
{
	int ret;

	ret = vo.StopAllChn();
	CHECK_ERROR(ret);

	switch (mode)
	{
	case Config::Scene::Mode::NORMAL_MODE:
	{
		ret = vo.StartChannel(0, {0, 0, 1920, 1080}, 0);
		CHECK_ERROR(ret);
		break;
	}
	case Config::Scene::Mode::PIP_MODE:
	{
		ret = vo.StartChannel(0, {0, 0, 1920, 1080}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(1, {1340, 660, 480, 320}, 1); //边距100px
		CHECK_ERROR(ret);
		break;
	}

	case Config::Scene::Mode::TWO:
	{
		ret = vo.StartChannel(0, {0, 0, 960, 1080}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(1, {960, 0, 960, 1080}, 0);
		CHECK_ERROR(ret);
		break;
	}
	case Config::Scene::Mode::THREE:
	{
		ret = vo.StartChannel(0, {0, 0, 1280, 1080}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(1, {1280, 0, 640, 540}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(2, {1280, 540, 640, 540}, 0);
		CHECK_ERROR(ret);
		break;
	}
	case Config::Scene::Mode::FOUR:
	{
		ret = vo.StartChannel(0, {0, 0, 960, 540}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(1, {960, 0, 960, 540}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(2, {0, 540, 960, 540}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(3, {960, 540, 960, 540}, 0);
		CHECK_ERROR(ret);
		break;
	}
	case Config::Scene::Mode::FOUR1:
	{
		ret = vo.StartChannel(0, {0, 0, 1440, 1080}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(1, {1440, 0, 480, 360}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(2, {1440, 360, 480, 360}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(3, {1440, 720, 480, 360}, 0);
		CHECK_ERROR(ret);
		break;
	}
	case Config::Scene::Mode::FIVE:
	{
		ret = vo.StartChannel(0, {0, 0, 960, 1080}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(1, {1440, 0, 480, 540}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(2, {1440, 540, 480, 540}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(3, {960, 0, 480, 540}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(4, {960, 540, 480, 540}, 0);
		CHECK_ERROR(ret);
		break;
	}
	case Config::Scene::Mode::SIX:
	{
		ret = vo.StartChannel(0, {0, 0, 640, 540}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(1, {640, 0, 640, 540}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(2, {1280, 0, 640, 540}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(3, {1280, 540, 640, 540}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(4, {640, 540, 640, 540}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(5, {0, 540, 640, 540}, 0);
		CHECK_ERROR(ret);
		break;
	}
	case Config::Scene::Mode::SIX1:
	{
		ret = vo.StartChannel(0, {0, 0, 1280, 720}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(1, {1280, 0, 640, 360}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(2, {1280, 360, 640, 360}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(3, {1280, 720, 640, 360}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(4, {640, 720, 640, 360}, 0);
		CHECK_ERROR(ret);
		ret = vo.StartChannel(5, {0, 720, 640, 360}, 0);
		CHECK_ERROR(ret);
		break;
	}

	default:
		RS_ASSERT(0);
		break;
	}

	return KSuccess;
}

static int InitializeVpss()
{
	int ret;
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
	ret = vpss_tea_fea_2vo.Initialize({10});
	CHECK_ERROR(ret);
	ret = vpss_stu_fea_2vo.Initialize({11});
	CHECK_ERROR(ret);

	return KSuccess;
}

static int InitializeVenc()
{
	int ret;
	ret = venc_tea_fea_record.Initialize({0,
										  0,
										  Config::Instance()->video_.record.width,
										  Config::Instance()->video_.record.height,
										  Config::Instance()->video_.frame_rate,
										  0,
										  Config::Instance()->video_.record.bitrate,
										  VENC_RC_MODE_H264CBR});
	CHECK_ERROR(ret);
	ret = venc_stu_fea_record.Initialize({1,
										  1,
										  Config::Instance()->video_.record.width,
										  Config::Instance()->video_.record.height,
										  Config::Instance()->video_.frame_rate,
										  0,
										  Config::Instance()->video_.record.bitrate,
										  VENC_RC_MODE_H264CBR});
	CHECK_ERROR(ret);
	ret = venc_tea_full_record.Initialize({2,
										   2,
										   Config::Instance()->video_.record.width,
										   Config::Instance()->video_.record.height,
										   Config::Instance()->video_.frame_rate,
										   0,
										   Config::Instance()->video_.record.bitrate,
										   VENC_RC_MODE_H264CBR});
	CHECK_ERROR(ret);
	ret = venc_stu_full_record.Initialize({3,
										   3,
										   Config::Instance()->video_.record.width,
										   Config::Instance()->video_.record.height,
										   Config::Instance()->video_.frame_rate,
										   0,
										   Config::Instance()->video_.record.bitrate,
										   VENC_RC_MODE_H264CBR});
	CHECK_ERROR(ret);
	ret = venc_black_board_record.Initialize({4,
											  4,
											  Config::Instance()->video_.record.width,
											  Config::Instance()->video_.record.height,
											  Config::Instance()->video_.frame_rate,
											  0,
											  Config::Instance()->video_.record.bitrate,
											  VENC_RC_MODE_H264CBR});
	CHECK_ERROR(ret);
	ret = venc_pc_record.Initialize({5,
									 5,
									 Config::Instance()->video_.record.width,
									 Config::Instance()->video_.record.height,
									 Config::Instance()->video_.frame_rate,
									 0,
									 Config::Instance()->video_.record.bitrate,
									 VENC_RC_MODE_H264CBR});
	CHECK_ERROR(ret);
	ret = venc_main_record.Initialize({6,
									   6,
									   Config::Instance()->video_.record.width,
									   Config::Instance()->video_.record.height,
									   Config::Instance()->video_.frame_rate,
									   0,
									   Config::Instance()->video_.record.bitrate,
									   VENC_RC_MODE_H264CBR});
	CHECK_ERROR(ret);

	ret = venc_tea_fea_live.Initialize({7,
										7,
										Config::Instance()->video_.live.width,
										Config::Instance()->video_.live.height,
										Config::Instance()->video_.frame_rate,
										0,
										Config::Instance()->video_.live.bitrate,
										VENC_RC_MODE_H264CBR});
	CHECK_ERROR(ret);
	ret = venc_stu_fea_live.Initialize({8,
										8,
										Config::Instance()->video_.live.width,
										Config::Instance()->video_.live.height,
										Config::Instance()->video_.frame_rate,
										0,
										Config::Instance()->video_.live.bitrate,
										VENC_RC_MODE_H264CBR});
	CHECK_ERROR(ret);
	ret = venc_tea_full_live.Initialize({9,
										 9,
										 Config::Instance()->video_.live.width,
										 Config::Instance()->video_.live.height,
										 Config::Instance()->video_.frame_rate,
										 0,
										 Config::Instance()->video_.live.bitrate,
										 VENC_RC_MODE_H264CBR});
	CHECK_ERROR(ret);
	ret = venc_stu_full_live.Initialize({10,
										 10,
										 Config::Instance()->video_.live.width,
										 Config::Instance()->video_.live.height,
										 Config::Instance()->video_.frame_rate,
										 0,
										 Config::Instance()->video_.live.bitrate,
										 VENC_RC_MODE_H264CBR});
	CHECK_ERROR(ret);
	ret = venc_black_board_live.Initialize({11,
											11,
											Config::Instance()->video_.live.width,
											Config::Instance()->video_.live.height,
											Config::Instance()->video_.frame_rate,
											0,
											Config::Instance()->video_.live.bitrate,
											VENC_RC_MODE_H264CBR});
	CHECK_ERROR(ret);
	ret = venc_pc_live.Initialize({12,
								   12,
								   Config::Instance()->video_.live.width,
								   Config::Instance()->video_.live.height,
								   Config::Instance()->video_.frame_rate,
								   0,
								   Config::Instance()->video_.live.bitrate,
								   VENC_RC_MODE_H264CBR});
	CHECK_ERROR(ret);
	ret = venc_main_live.Initialize({13,
									 13,
									 Config::Instance()->video_.live.width,
									 Config::Instance()->video_.live.height,
									 Config::Instance()->video_.frame_rate,
									 0,
									 Config::Instance()->video_.live.bitrate,
									 VENC_RC_MODE_H264CBR});
	CHECK_ERROR(ret);

	return KSuccess;
}

static int InitializeVdec()
{
	int ret;
	ret = vdec_tea_full.Initialize({0, RS_MAX_WIDTH, RS_MAX_HEIGHT});
	CHECK_ERROR(ret);
	ret = vdec_stu_full.Initialize({1, RS_MAX_WIDTH, RS_MAX_HEIGHT});
	CHECK_ERROR(ret);
	ret = vdec_black_board.Initialize({2, RS_MAX_WIDTH, RS_MAX_HEIGHT});
	CHECK_ERROR(ret);
	ret = vdec_pc.Initialize({3, RS_MAX_WIDTH, RS_MAX_HEIGHT});
	CHECK_ERROR(ret);

	return KSuccess;
}

static int InitializeVo()
{
	int ret;
	ret = vo_disp.Initialize({0, VO_INTF_BT1120 | VO_INTF_VGA | VO_INTF_HDMI, Config::Instance()->system_.disp_vo_intf_sync});
	CHECK_ERROR(ret);

	ret = vo_tea_fea.Initialize({10, 0, VO_OUTPUT_1080P30});
	CHECK_ERROR(ret);

	ret = vo_stu_fea.Initialize({11, 0, VO_OUTPUT_1080P30});
	CHECK_ERROR(ret);

	ret = vo_main.Initialize({12, 0, VO_OUTPUT_1080P30});
	CHECK_ERROR(ret);

	for (const Config::System::Chn &tmp : Config::Instance()->system_.chns)
	{
		ret = vo_disp.StartChannel(tmp.chn, tmp.rect, 0);
		CHECK_ERROR(ret);
	}

	ret = vo_tea_fea.StartChannel(0, {0, 0, 1920, 1080}, 0);
	CHECK_ERROR(ret);

	ret = vo_stu_fea.StartChannel(0, {0, 0, 1920, 1080}, 0);
	CHECK_ERROR(ret);

	ret = SetSceneMode(vo_main, Config::Instance()->scene_.mode);
	CHECK_ERROR(ret);

	return KSuccess;
}

int32_t main(int32_t argc, char **argv)
{
	int ret;

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

	ret = MPPSystem::Instance()->Initialize(RS_MEM_BLK_NUM);
	CHECK_ERROR_EXT(ret);

	//初始化vpss
	ret = InitializeVpss();
	CHECK_ERROR_EXT(ret);

	//初始化venc
	ret = InitializeVenc();
	CHECK_ERROR_EXT(ret);

	//初始化vdec
	ret = InitializeVdec();
	CHECK_ERROR_EXT(ret);

	//初始化vo
	ret = InitializeVo();
	CHECK_ERROR_EXT(ret);

	//绑定VI与VPSS
	ret = MPPSystem::Bind<HI_ID_VIU, HI_ID_VPSS>(0, 8, 10, 0);
	CHECK_ERROR_EXT(ret);
	ret = MPPSystem::Bind<HI_ID_VIU, HI_ID_VPSS>(0, 4, 11, 0);
	CHECK_ERROR_EXT(ret);
	//绑定VPSS与虚拟VO
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(10, VPSS_BYPASS_CHN, 10, 0);
	CHECK_ERROR_EXT(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(11, VPSS_BYPASS_CHN, 11, 0);
	CHECK_ERROR_EXT(ret);

	ret = MPPSystem::Bind<HI_ID_VOU, HI_ID_VPSS>(10, 0, 0, 0);
	CHECK_ERROR_EXT(ret);
	ret = MPPSystem::Bind<HI_ID_VOU, HI_ID_VPSS>(11, 0, 1, 0);
	CHECK_ERROR_EXT(ret);
	ret = MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 0, 2, 0);
	CHECK_ERROR_EXT(ret);
	ret = MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 1, 3, 0);
	CHECK_ERROR_EXT(ret);
	ret = MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 2, 4, 0);
	CHECK_ERROR_EXT(ret);
	ret = MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 3, 5, 0);
	CHECK_ERROR_EXT(ret);

	ret = MPPSystem::Bind<HI_ID_VOU, HI_ID_VPSS>(12, 0, 6, 0);
	CHECK_ERROR_EXT(ret);

	for (auto it = Config::Instance()->scene_.mapping.begin(); it != Config::Instance()->scene_.mapping.end(); it++)
	{
		printf("%d , %d \n", it->second, it->first);
		switch (it->second)
		{
		case TEA_FEATURE:
		{
			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(0, VPSS_BYPASS_CHN, 12, it->first);
			CHECK_ERROR_EXT(ret);
			break;
		}
		case STU_FEATURE:
		{
			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(1, VPSS_BYPASS_CHN, 12, it->first);
			CHECK_ERROR_EXT(ret);
			break;
		}
		case TEA_FULL_VIEW:
		{
			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(2, VPSS_BYPASS_CHN, 12, it->first);
			CHECK_ERROR_EXT(ret);
			break;
		}

		case STU_FULL_VIEW:
		{
			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(3, VPSS_BYPASS_CHN, 12, it->first);
			CHECK_ERROR_EXT(ret);
			break;
		}

		case PC_CAPTURE:
		{
			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(4, VPSS_BYPASS_CHN, 12, it->first);
			CHECK_ERROR_EXT(ret);
			break;
		}
		case BLACK_BOARD_FEATURE:
		{
			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(5, VPSS_BYPASS_CHN, 12, it->first);
			CHECK_ERROR_EXT(ret);
			break;
		}

		case MAIN:
		{
			ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(g_CurMainScene, VPSS_BYPASS_CHN, 12, it->first);
			CHECK_ERROR_EXT(ret);
			break;
		}
		default:
		{
			RS_ASSERT(0);
		}
		}
	}

	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(6, 1, 0, 0);
	CHECK_ERROR_EXT(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(0, 1, 0, 1);
	CHECK_ERROR_EXT(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(1, 1, 0, 2);
	CHECK_ERROR_EXT(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(2, 1, 0, 3);
	CHECK_ERROR_EXT(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(3, 1, 0, 4);
	CHECK_ERROR_EXT(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(4, 1, 0, 5);
	CHECK_ERROR_EXT(ret);

	ret = PCIVComm::Instance()->Initialize();
	CHECK_ERROR(ret);

	ret = PCIVTrans::Instance()->Initialize(PCIVComm::Instance());
	CHECK_ERROR(ret);
	PCIVTrans::Instance()->AddVideoSink(&vdec_tea_full);
	PCIVTrans::Instance()->AddVideoSink(&vdec_stu_full);
	PCIVTrans::Instance()->AddVideoSink(&vdec_black_board);
	PCIVTrans::Instance()->AddVideoSink(&vdec_pc);

	ret = SigDetect::Instance()->Initialize(PCIVComm::Instance(), MODE_HDMI);
	CHECK_ERROR(ret);
	SigDetect::Instance()->AddVIFmtListener(&vi_tea_fea);
	SigDetect::Instance()->AddVIFmtListener(&vi_stu_fea);

	// ret = vo_disp.StartChannel(0, {0, 0, 640, 360}, 0);
	// CHECK_ERROR(ret);
	// ret = vo_disp.StartChannel(1, {640, 0, 640, 360}, 0);
	// CHECK_ERROR(ret);
	// ret = vo_disp.StartChannel(2, {1280, 0, 640, 360}, 0);
	// CHECK_ERROR(ret);
	// ret = vo_disp.StartChannel(3, {0, 360, 640, 360}, 0);
	// CHECK_ERROR(ret);
	// ret = vo_disp.StartChannel(4, {640, 360, 640, 360}, 0);
	// CHECK_ERROR(ret);
	// ret = vo_disp.StartChannel(5, {1280, 360, 640, 360}, 0);
	// CHECK_ERROR(ret);

	// ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(6, 1, 0, 0);
	// CHECK_ERROR(ret);
	// ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(1, 1, 0, 1);
	// CHECK_ERROR(ret);
	// ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(2, 1, 0, 2);
	// CHECK_ERROR(ret);
	// ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(3, 1, 0, 3);
	// CHECK_ERROR(ret);
	// ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(4, 1, 0, 4);
	// CHECK_ERROR(ret);
	// ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(5, 1, 0, 5);
	// CHECK_ERROR(ret);

	while (g_Run)
		sleep(10000);

	SigDetect::Instance()->Close();
	PCIVTrans::Instance()->Close();
	PCIVComm::Instance()->Close();

	// sleep(20);
	return 0;
}
