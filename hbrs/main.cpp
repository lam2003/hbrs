#include "system/mpp.h"
#include "system/pciv_comm.h"
#include "system/pciv_trans.h"
#include "system/vi.h"
#include "system/vo.h"
#include "system/vpss.h"
#include "system/venc.h"
#include "system/vdec.h"
// #include "common/buffer.h"

#include "system/sig_detect.h"

using namespace rs;

#define CHACK_ERROR(a)                                                                  \
	if (KSuccess != a)                                                                  \
	{                                                                                   \
		log_e("error:%s", make_error_code(static_cast<err_code>(a)).message().c_str()); \
		return a;                                                                       \
	}

static bool g_Run = true;

int32_t main(int32_t argc, char **argv)
{



	MPPSystem::Instance()->Initialize(20);
		VideoInput vi;
	vi.Initialize({2,4,1920,1080,false});

	// VideoOutput vo;
	// vo.Initialize({0, VO_INTF_BT1120 | VO_INTF_VGA | VO_INTF_HDMI, VO_OUTPUT_1280x800_60});
	// vo.StartChn({{0, 0, 1280, 720}, 0, 0,61});

	PCIVComm::Instance()->Initialize();

	// pciv::Msg msg;
	// msg.type = pciv::Msg::Type::CONF_ADV7842;

	// pciv::Adv7842Conf *conf = reinterpret_cast<pciv::Adv7842Conf *>(msg.data);
	// conf->mode = 0;
	// PCIVComm::Instance()->Send(1, 0, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));

	// VideoDecode vdec;
	// vdec.Initialize({3, 1920, 1080});

	// VideoProcess vpss;
	// vpss.Initialize({0});
	// vpss.SetChnSize(1, {1920, 1080});

	// VPSS_FRAME_RATE_S ff;
	// ff.s32SrcFrmRate = 60;
	// ff.s32DstFrmRate = 30;

	// HI_MPI_VPSS_SetGrpFrameRate(0,
	// 							&ff);

	// MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 3, 0, 0);
	// MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(0, 1, 0, 0);

	
	PCIVTrans::Instance()->Initialize(PCIVComm::Instance());
	// PCIVTrans::Instance()->AddVideoSink(&vdec);
	SigDetect::Instance()->Initialize(PCIVComm::Instance());

	while (1)
		sleep(10);
	return 0;
}
