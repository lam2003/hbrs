#include "system/vm.h"
#include "system/vi.h"
#include "system/vo.h"
#include "system/vpss.h"
#include "system/venc.h"
#include "system/vdec.h"
#include "system/pciv_comm.h"
#include "system/pciv_trans.h"
#include "system/sig_detect.h"
#include "system/mpp.h"
#include "common/err_code.h"
#include "common/config.h"

namespace rs
{

VideoManager::VideoManager() : init_(false)
{
}
VideoManager::~VideoManager()
{
}
int VideoManager::Initialize()
{
    if (init_)
        return KInitialized;
    //create static module
    vi_arr_.resize(2);
    for (int i = 0; i < 2; i++)
        vi_arr_[i] = std::make_shared<VIHelper>(4 - (i * 2), 8 - (i * 4)); //4,8、2,4

    vo_arr_.resize(2);
    for (int i = 0; i < 2; i++)
        vo_arr_[i] = std::make_shared<VideoOutput>();

    vdec_arr_.resize(4);
    for (int i = 0; i < 4; i++)
        vdec_arr_[i] = std::make_shared<VideoDecode>();

    vpss_arr_.resize(7);
    for (int i = 0; i < 7; i++)
        vpss_arr_[i] = std::make_shared<VideoProcess>();

    //create dynamic module
    venc_arr_.resize(8);
    for (int i = 0; i < 8; i++)
        venc_arr_[i] = std::make_shared<VideoEncode>();

    //create static module
    main_vo_ = std::make_shared<VideoOutput>();
    pciv_comm_ = std::make_shared<PCIVComm>();
    pciv_trans_ = std::make_shared<PCIVTrans>();
    sig_detect_ = std::make_shared<SigDetect>();

    // initialize
    for (int i = 0; i < 2; i++)
        vi_arr_[i]->Start(RS_MAX_WIDTH, RS_MAX_HEIGHT, false); //预设摄像头采集时序1080P

    for (int i = 0; i < 2; i++)
    {
        vo_arr_[i]->Initialize({10 + i, 0, VO_OUTPUT_1080P25});
        vo_arr_[i]->StartChannel(0, {0, 0, RS_MAX_WIDTH, RS_MAX_HEIGHT}, 0);
    }
    for (int i = 0; i < 4; i++)
        vdec_arr_[i]->Initialize({i, RS_MAX_WIDTH, RS_MAX_HEIGHT});

    for (int i = 0; i < 7; i++)
        vpss_arr_[i]->Initialize({i});

    main_vo_->Initialize({12, 0, VO_OUTPUT_1080P25});
    main_vo_->StartChannel(0, {0, 0, RS_MAX_WIDTH, RS_MAX_HEIGHT}, 0);

    pciv_comm_->Initialize();

    for (int i = 0; i < 4; i++)
        pciv_trans_->AddVideoSink(vdec_arr_[i]);
    pciv_trans_->Initialize(pciv_comm_);

    for (int i = 0; i < 2; i++)
        sig_detect_->AddVIFmtListener(vi_arr_[i]);
    sig_detect_->Initialize(pciv_comm_, Config::Instance()->system_.pc_capture_mode);

    MPPSystem::Bind<HI_ID_VIU, HI_ID_VOU>(0, 8, 10, 0);
    MPPSystem::Bind<HI_ID_VIU, HI_ID_VOU>(0, 4, 11, 0);
    MPPSystem::Bind<HI_ID_VOU, HI_ID_VPSS>(10, 0, 0, 0);
    MPPSystem::Bind<HI_ID_VOU, HI_ID_VPSS>(11, 0, 1, 0);
    MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 0, 2, 0);
    MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 1, 3, 0);
    MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 2, 4, 0);
    MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 3, 5, 0);
    MPPSystem::Bind<HI_ID_VOU, HI_ID_VPSS>(12, 0, 6, 0);

    init_ = true;

    StartVideoEncode();
    return KSuccess;
}

void VideoManager::StartVideoEncode()
{
    if (!init_)
        return;

    if (!Config::Instance()->IsResourceMode())
    {
        for (int i = 0; i < 8; i++)
        {
            if (i == 7)
            {
                venc_arr_[i]->Initialize({i, i, Config::Instance()->video_.normal_record_width, Config::Instance()->video_.normal_record_height, 25, 25, 0, Config::Instance()->video_.normal_record_bitrate, VENC_RC_MODE_H264CBR, true});
                vpss_arr_[i]->StartUserChannel(3, {0, 0, (HI_U32)Config::Instance()->video_.normal_record_width, (HI_U32)Config::Instance()->video_.normal_record_height});
                MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(6, 3, i, 0);
            }
            else
            {
                venc_arr_[i]->Initialize({i, i, Config::Instance()->video_.normal_live_width, Config::Instance()->video_.normal_live_height, 25, 25, 0, Config::Instance()->video_.normal_live_bitrate, VENC_RC_MODE_H264CBR, true});
                vpss_arr_[i]->StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.normal_live_width, (HI_U32)Config::Instance()->video_.normal_live_height});
                MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(i, 1, i, 0);
            }
        }
    }
    else
    {
        for (int i = 0; i < 7; i++)
        {
            venc_arr_[i]->Initialize({i, i, Config::Instance()->video_.res_width, Config::Instance()->video_.res_height, 25, 25, 0, Config::Instance()->video_.res_bitrate, VENC_RC_MODE_H264CBR, true});
            vpss_arr_[i]->StartUserChannel(1, {0, 0, (HI_U32)Config::Instance()->video_.res_width, (HI_U32)Config::Instance()->video_.res_height});
            MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(i, 1, i, 0);
        }
    }
}

void VideoManager::StopVideoEncode()
{
}

void VideoManager::Close()
{
    if (!init_)
        return;
    // disconnect
    MPPSystem::UnBind<HI_ID_VOU, HI_ID_VPSS>(12, 0, 6, 0);
    MPPSystem::UnBind<HI_ID_VDEC, HI_ID_VPSS>(0, 3, 5, 0);
    MPPSystem::UnBind<HI_ID_VDEC, HI_ID_VPSS>(0, 2, 4, 0);
    MPPSystem::UnBind<HI_ID_VDEC, HI_ID_VPSS>(0, 1, 3, 0);
    MPPSystem::UnBind<HI_ID_VDEC, HI_ID_VPSS>(0, 0, 2, 0);
    MPPSystem::UnBind<HI_ID_VOU, HI_ID_VPSS>(11, 0, 1, 0);
    MPPSystem::UnBind<HI_ID_VOU, HI_ID_VPSS>(10, 0, 0, 0);
    MPPSystem::UnBind<HI_ID_VIU, HI_ID_VOU>(0, 4, 11, 0);
    MPPSystem::UnBind<HI_ID_VIU, HI_ID_VOU>(0, 8, 10, 0);

    sig_detect_->Close();
    sig_detect_->RemoveAllVIFmtListener();
    pciv_trans_->Close();
    pciv_comm_->Close();
    pciv_trans_->RemoveAllVideoSink();
    main_vo_->Close();

    for (int i = 0; i < 7; i++)
        vpss_arr_[i]->Close();

    for (int i = 0; i < 4; i++)
        vdec_arr_[i]->Close();

    for (int i = 0; i < 2; i++)
        vo_arr_[i]->Close();

    for (int i = 0; i < 2; i++)
        vi_arr_[i]->Stop();

    sig_detect_.reset();
    sig_detect_ = nullptr;
    pciv_trans_.reset();
    pciv_trans_ = nullptr;
    pciv_comm_.reset();
    pciv_comm_ = nullptr;
    main_vo_.reset();
    main_vo_ = nullptr;

    for (int i = 0; i < 8; i++)
    {
        venc_arr_[i].reset();
        venc_arr_[i] = nullptr;
    }

    for (int i = 0; i < 7; i++)
    {
        vpss_arr_[i].reset();
        vpss_arr_[i] = nullptr;
    }

    for (int i = 0; i < 4; i++)
    {
        vdec_arr_[i].reset();
        vdec_arr_[i] = nullptr;
    }

    for (int i = 0; i < 2; i++)
    {
        vo_arr_[i].reset();
        vo_arr_[i] = nullptr;
    }
    
    for (int i = 0; i < 2; i++)
    {
        vi_arr_[i].reset();
        vi_arr_[i] = nullptr;
    }

    init_ = false;
}
} // namespace rs