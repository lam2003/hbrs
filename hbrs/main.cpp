#include "system/mpp.h"
#include "system/sig_detect.h"
#include "system/pciv_comm.h"
#include "system/pciv_trans.h"
#include "system/vi.h"
#include "system/vo.h"
#include "system/vpss.h"
#include "system/venc.h"
#include "system/vdec.h"
#include "common/buffer.h"

using namespace rs;

#define CHECK_ERROR(a)                                                                  \
	if (KSuccess != a)                                                                  \
	{                                                                                   \
		log_e("error:%s", make_error_code(static_cast<err_code>(a)).message().c_str()); \
		return a;                                                                       \
	}

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

	signal(SIGINT, SignalHandler);

	ret = MPPSystem::Instance()->Initialize(RS_MEM_BLK_NUM);
	CHECK_ERROR(ret);

	VIHelper vi_tea_fea(4, 8);
	VIHelper vi_stu_fea(2, 4);

	VideoDecode vdec_tea_full;
	VideoDecode vdec_stu_full;
	VideoDecode vdec_black_board;
	VideoDecode vdec_pc;

	VideoProcess vpss_tea_fea;
	VideoProcess vpss_stu_fea;
	VideoProcess vpss_tea_full;
	VideoProcess vpss_stu_full;
	VideoProcess vpss_black_board;
	VideoProcess vpss_pc;
	VideoProcess vpss_tea_fea_2vo;
	VideoProcess vpss_stu_fea_2vo;

	VideoOutput vo_tea_fea;
	VideoOutput vo_stu_fea;
	VideoOutput vo_main;
	VideoOutput vo_disp;

	VideoEncode venc_tea_fea;
	VideoEncode venc_stu_fea;
	VideoEncode venc_tea_full;
	VideoEncode venc_stu_full;
	VideoEncode venc_black_board;
	VideoEncode venc_pc;
	VideoEncode venc_main;

	//初始化与虚拟vo绑定的vpss
	ret = vpss_tea_fea_2vo.Initialize({10});
	CHECK_ERROR(ret);
	ret = vpss_stu_fea_2vo.Initialize({11});
	CHECK_ERROR(ret);
	//配置vpss通道1
	ret = vpss_tea_fea_2vo.StartUserChannel(1, {RS_MAX_WIDTH, RS_MAX_HEIGHT});
	CHECK_ERROR(ret);
	ret = vpss_stu_fea_2vo.StartUserChannel(1, {RS_MAX_WIDTH, RS_MAX_HEIGHT});
	CHECK_ERROR(ret);
	//初始化虚拟vo
	ret = vo_tea_fea.Initialize({10, 0, VO_OUTPUT_1080P30});
	CHECK_ERROR(ret);
	ret = vo_stu_fea.Initialize({11, 0, VO_OUTPUT_1080P30});
	//配置虚拟vo通道0
	ret = vo_tea_fea.StartChannel(0, {0, 0, RS_MAX_WIDTH, RS_MAX_HEIGHT}, 0);
	CHECK_ERROR(ret);
	ret = vo_stu_fea.StartChannel(0, {0, 0, RS_MAX_WIDTH, RS_MAX_HEIGHT}, 0);
	CHECK_ERROR(ret);
	//初始化vdec
	ret = vdec_tea_full.Initialize({0, RS_MAX_WIDTH, RS_MAX_HEIGHT});
	CHECK_ERROR(ret);
	ret = vdec_stu_full.Initialize({1, RS_MAX_WIDTH, RS_MAX_HEIGHT});
	CHECK_ERROR(ret);
	ret = vdec_black_board.Initialize({2, RS_MAX_WIDTH, RS_MAX_HEIGHT});
	CHECK_ERROR(ret);
	ret = vdec_pc.Initialize({3, RS_MAX_WIDTH, RS_MAX_HEIGHT});
	CHECK_ERROR(ret);
	//初始化与venc绑定的vpss
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

	//绑定VI与VPSS
	ret = MPPSystem::Bind<HI_ID_VIU, HI_ID_VPSS>(0, 8, 10, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VIU, HI_ID_VPSS>(0, 4, 11, 0);
	CHECK_ERROR(ret);
	//绑定VPSS与虚拟VO
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(10, 1, 10, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(11, 1, 11, 0);
	CHECK_ERROR(ret);
	//绑定虚拟VO与VPSS
	ret = MPPSystem::Bind<HI_ID_VOU, HI_ID_VPSS>(10, 0, 0, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VOU, HI_ID_VPSS>(11, 0, 1, 0);
	CHECK_ERROR(ret);
	//绑定VDEC与VPSS
	ret = MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 0, 2, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 1, 3, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 2, 4, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 3, 5, 0);
	CHECK_ERROR(ret);

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

	ret = vo_disp.Initialize({0, VO_INTF_BT1120 | VO_INTF_VGA | VO_INTF_HDMI, VO_OUTPUT_1080P60});
	CHECK_ERROR(ret);

	ret = vo_disp.StartChannel(0, {0, 0, 640, 360}, 0);
	CHECK_ERROR(ret);
	ret = vo_disp.StartChannel(1, {640, 0, 640, 360}, 0);
	CHECK_ERROR(ret);
	ret = vo_disp.StartChannel(2, {1280, 0, 640, 360}, 0);
	CHECK_ERROR(ret);
	ret = vo_disp.StartChannel(3, {0, 360, 640, 360}, 0);
	CHECK_ERROR(ret);
	ret = vo_disp.StartChannel(4, {640, 360, 640, 360}, 0);
	CHECK_ERROR(ret);
	ret = vo_disp.StartChannel(5, {1280, 360, 640, 360}, 0);
	CHECK_ERROR(ret);

	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(0, 1, 0, 0);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(1, 1, 0, 1);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(2, 1, 0, 2);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(3, 1, 0, 3);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(4, 1, 0, 4);
	CHECK_ERROR(ret);
	ret = MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(5, 1, 0, 5);
	CHECK_ERROR(ret);

	while (g_Run)
		sleep(10000);

	SigDetect::Instance()->Close();
	PCIVTrans::Instance()->Close();
	PCIVComm::Instance()->Close();

	return 0;
}
