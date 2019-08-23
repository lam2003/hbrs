#include "system/vm.h"
#include "system/vi.h"
#include "system/vo.h"
#include "system/vpss.h"
#include "system/venc.h"
#include "system/vdec.h"
#include "system/ai.h"
#include "system/aenc.h"
#include "system/ao.h"
#include "system/pciv_comm.h"
#include "system/pciv_trans.h"
#include "system/sig_detect.h"
#include "system/mpp.h"
#include "common/err_code.h"
#include "common/utils.h"

namespace rs
{

VideoManager::VideoManager() : display_vo_(nullptr),
                               main_vo_(nullptr),
                               ai_(nullptr),
                               aenc_(nullptr),
                               ao_(nullptr),
                               pciv_comm_(nullptr),
                               pciv_trans_(nullptr),
                               sig_detect_(nullptr),
                               main_screen_(TEA_FEA),
                               encode_stared_(false),
                               display_screen_started_(false),
                               main_screen_started_(false),
                               local_live_started_(false),
                               remote_live_started_(false),
                               record_started_(false),
                               pip_changed_(false),
                               init_(false)
{
}
VideoManager::~VideoManager()
{
}
int VideoManager::Initialize()
{
    if (init_)
        return KInitialized;

    //################################################
    // 初始化视频
    //################################################
    vi_arr_.resize(2);
    for (int i = 0; i < 2; i++)
        vi_arr_[i] = std::make_shared<VIHelper>(4 - (i * 2), 8 - (i * 4));

    vpss_tmp_arr_.resize(2);
    for (int i = 0; i < 2; i++)
        vpss_tmp_arr_[i] = std::make_shared<VideoProcess>();

    vo_arr_.resize(2);
    for (int i = 0; i < 2; i++)
        vo_arr_[i] = std::make_shared<VideoOutput>();

    vdec_arr_.resize(4);
    for (int i = 0; i < 4; i++)
        vdec_arr_[i] = std::make_shared<VideoDecode>();

    vpss_arr_.resize(7);
    for (int i = 0; i < 7; i++)
        vpss_arr_[i] = std::make_shared<VideoProcess>();

    venc_arr_.resize(8);
    for (int i = 0; i < 8; i++)
        venc_arr_[i] = std::make_shared<VideoEncode>();

    display_vo_ = std::make_shared<VideoOutput>();

    main_vo_ = std::make_shared<VideoOutput>();

    pciv_comm_ = std::make_shared<PCIVComm>();

    pciv_trans_ = std::make_shared<PCIVTrans>();

    sig_detect_ = std::make_shared<SigDetect>();

    for (int i = 0; i < 2; i++)
    {
        vi_arr_[i]->Start(RS_MAX_WIDTH, RS_MAX_HEIGHT, false); //预设摄像头采集时序1080P
        vi_arr_[i]->SetVideoOutput(vo_arr_[i]);
    }
    for (int i = 0; i < 2; i++)
    {
        vpss_tmp_arr_[i]->Initialize({10 + i});
        vpss_tmp_arr_[i]->StartUserChannel(1, {0, 0, RS_MAX_WIDTH, RS_MAX_HEIGHT});
    }

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
    sig_detect_->Initialize(pciv_comm_, CONFIG->adv7842_.pc_capture_mode);

    MPPSystem::Bind<HI_ID_VIU, HI_ID_VPSS>(0, 8, 10, 0);

    MPPSystem::Bind<HI_ID_VIU, HI_ID_VPSS>(0, 4, 11, 0);

    MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(10, 1, 10, 0);

    MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(11, 1, 11, 0);

    MPPSystem::Bind<HI_ID_VOU, HI_ID_VPSS>(10, 0, 0, 0);

    MPPSystem::Bind<HI_ID_VOU, HI_ID_VPSS>(11, 0, 1, 0);

    MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 0, 2, 0);

    MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 1, 3, 0);

    MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 2, 4, 0);

    MPPSystem::Bind<HI_ID_VDEC, HI_ID_VPSS>(0, 3, 5, 0);

    MPPSystem::Bind<HI_ID_VOU, HI_ID_VPSS>(12, 0, 6, 0);
    //################################################
    // 初始化音频
    //################################################
    ai_ = std::make_shared<AudioInput>();

    ao_ = std::make_shared<AudioOutput>();

    aenc_ = std::make_shared<AudioEncode>();

    ai_->Initialize({4, 0});

    ao_->Initialize({4, 0});

    aenc_->Initialize();

    ai_->AddAudioSink(aenc_);

    MPPSystem::Bind<HI_ID_AI, HI_ID_AO>(4, 0, 4, 0);

    //##############################################
    //初始化功能模块
    //##############################################

    live_arr_.resize(8);
    for (int i = 0; i < 8; i++)
        live_arr_[i] = std::make_shared<RTMPLive>();

    record_arr_.resize(8);
    for (int i = 0; i < 8; i++)
        record_arr_[i] = std::make_shared<MP4Record>();

    init_ = true;

    StartMainScreen(CONFIG->scene_);

    StartDisplayScreen(CONFIG->display_);

    StartVideoEncode(CONFIG->video_);

    StartLocalLive(CONFIG->local_lives_);

    StartRemoteLive(CONFIG->remote_live_);

    return KSuccess;
}

void VideoManager::Close()
{

    if (!init_)
        return;

    CloseRemoteLive();

    CloseLocalLive();

    CloseVideoEncode();

    CloseDisplayScreen();

    CloseMainScreen();

    //################################################
    //去初始化功能模块
    //################################################

    for (int i = 0; i < 8; i++)
    {
        record_arr_[i].reset();
        record_arr_[i] = nullptr;
    }

    for (int i = 0; i < 8; i++)
    {
        live_arr_[i].reset();
        live_arr_[i] = nullptr;
    }

    //################################################
    // 去初始化音频
    //################################################
    MPPSystem::UnBind<HI_ID_AI, HI_ID_AO>(4, 0, 4, 0);

    ai_->RemoveAllAudioSink();

    aenc_->Close();

    ao_->Close();

    ai_->Close();

    aenc_.reset();
    aenc_ = nullptr;

    ao_.reset();
    ao_ = nullptr;

    ai_.reset();
    ai_ = nullptr;

    //################################################
    // 去初始化视频
    //################################################
    MPPSystem::UnBind<HI_ID_VOU, HI_ID_VPSS>(12, 0, 6, 0);

    MPPSystem::UnBind<HI_ID_VDEC, HI_ID_VPSS>(0, 3, 5, 0);

    MPPSystem::UnBind<HI_ID_VDEC, HI_ID_VPSS>(0, 2, 4, 0);

    MPPSystem::UnBind<HI_ID_VDEC, HI_ID_VPSS>(0, 1, 3, 0);

    MPPSystem::UnBind<HI_ID_VDEC, HI_ID_VPSS>(0, 0, 2, 0);

    MPPSystem::UnBind<HI_ID_VOU, HI_ID_VPSS>(11, 0, 1, 0);

    MPPSystem::UnBind<HI_ID_VOU, HI_ID_VPSS>(10, 0, 0, 0);

    MPPSystem::UnBind<HI_ID_VPSS, HI_ID_VOU>(11, 1, 11, 0);

    MPPSystem::UnBind<HI_ID_VPSS, HI_ID_VOU>(10, 1, 10, 0);

    MPPSystem::UnBind<HI_ID_VIU, HI_ID_VPSS>(0, 4, 11, 0);

    MPPSystem::UnBind<HI_ID_VIU, HI_ID_VPSS>(0, 8, 10, 0);

    sig_detect_->Close();
    sig_detect_->RemoveAllVIFmtListener();

    pciv_trans_->Close();
    pciv_trans_->RemoveAllVideoSink();

    pciv_comm_->Close();

    main_vo_->Close();

    for (int i = 0; i < 7; i++)
        vpss_arr_[i]->Close();

    for (int i = 0; i < 4; i++)
        vdec_arr_[i]->Close();

    for (int i = 0; i < 2; i++)
        vo_arr_[i]->Close();

    for (int i = 0; i < 2; i++)
        vpss_tmp_arr_[i]->Close();

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

    display_vo_.reset();
    display_vo_ = nullptr;

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
        vpss_tmp_arr_[i].reset();
        vpss_tmp_arr_[i] = nullptr;
    }

    for (int i = 0; i < 2; i++)
    {
        vi_arr_[i].reset();
        vi_arr_[i] = nullptr;
    }

    init_ = false;
}

void VideoManager::StartLocalLive(const Config::LocalLive &local_lives)
{
    if (!init_ && local_live_started_)
        return;

    if (local_lives.lives.size() == 0)
        return;

    for (const std::pair<RS_SCENE, rtmp::Params> &item : local_lives.lives)
    {
        RS_SCENE scene = item.first;
        rtmp::Params rtmp_params = item.second;
        if (scene != MAIN)
        {
            live_arr_[scene]->Initialize(rtmp_params);
        }
        else
        {
            live_arr_[scene]->Initialize(rtmp_params);
            aenc_->AddAudioSink(live_arr_[scene]);
        }
        venc_arr_[scene]->AddVideoSink(live_arr_[scene]);
    }

    CONFIG->local_lives_ = local_lives;
    CONFIG->WriteToFile();
    local_live_started_ = true;
}

void VideoManager::CloseLocalLive()
{
    if (!init_ && !local_live_started_)
        return;

    for (const std::pair<RS_SCENE, rtmp::Params> &item : CONFIG->local_lives_.lives)
    {
        RS_SCENE scene = item.first;
        venc_arr_[scene]->RemoveVideoSink(live_arr_[scene]);

        if (scene != MAIN)
        {
            live_arr_[scene]->Close();
        }
        else
        {
            aenc_->RemoveAudioSink(live_arr_[scene]);
            live_arr_[scene]->Close();
        }
    }

    CONFIG->local_lives_.lives.clear();
    CONFIG->WriteToFile();
    local_live_started_ = false;
}

void VideoManager::StartRemoteLive(const Config::RemoteLive &remote_live)
{
    if (!init_ && remote_live_started_)
        return;

    if (remote_live.live.url == "")
        return;

    live_arr_[MAIN2]->Initialize(remote_live.live);
    aenc_->AddAudioSink(live_arr_[MAIN2]);
    venc_arr_[MAIN2]->AddVideoSink(live_arr_[MAIN2]);

    CONFIG->remote_live_ = remote_live;
    CONFIG->WriteToFile();
    remote_live_started_ = true;
}

void VideoManager::CloseRemoteLive()
{
    if (!init_ && !remote_live_started_)
        return;

    venc_arr_[MAIN2]->RemoveVideoSink(live_arr_[MAIN2]);
    aenc_->RemoveAudioSink(live_arr_[MAIN2]);
    live_arr_[MAIN2]->Close();

    CONFIG->remote_live_.live.url = "";
    CONFIG->WriteToFile();

    remote_live_started_ = false;
}

void VideoManager::StartRecord(const Config::Record &records)
{
    if (record_started_)
        return;

    if (records.records.size() == 0)
        return;
    if (!CONFIG->IsResourceMode())
    {
        if (records.records.size() != 1 || records.records[0].first != MAIN)
            return;
    }

    for (const std::pair<RS_SCENE, mp4::Params> &item : records.records)
    {
        RS_SCENE scene = item.first;
        mp4::Params record_params = item.second;

        if (scene == MAIN && !CONFIG->IsResourceMode())
        {
            record_arr_[MAIN2]->Initialize(record_params);
            venc_arr_[MAIN2]->AddVideoSink(record_arr_[MAIN2]);
            aenc_->AddAudioSink(record_arr_[MAIN2]);
        }
        else
        {
            record_arr_[scene]->Initialize(record_params);
            venc_arr_[scene]->AddVideoSink(record_arr_[scene]);
            aenc_->AddAudioSink(record_arr_[scene]);
        }
    }

    CONFIG->records_ = records;
    record_started_ = true;
}

void VideoManager::CloseRecord()
{
    if (!record_started_)
        return;

    for (const std::pair<RS_SCENE, mp4::Params> &item : CONFIG->records_.records)
    {
        RS_SCENE scene = item.first;

        if (scene == MAIN && !CONFIG->IsResourceMode())
        {
            aenc_->RemoveAudioSink(record_arr_[MAIN2]);
            venc_arr_[MAIN2]->RemoveVideoSink(record_arr_[MAIN2]);
            record_arr_[MAIN2]->Close();
        }
        else
        {
            aenc_->RemoveAudioSink(record_arr_[scene]);
            venc_arr_[scene]->RemoveVideoSink(record_arr_[scene]);
            record_arr_[scene]->Close();
        }
    }

    CONFIG->records_.records.clear();
    record_started_ = false;
}

void VideoManager::StartMainScreen(const Config::Scene &scene_conf)
{
    if (!init_ && main_screen_started_)
        return;

    std::map<int, std::pair<RECT_S, int>> main_pos = VideoOutput::GetScenePos(scene_conf.mode);
    for (auto it = scene_conf.mapping.begin(); it != scene_conf.mapping.end(); it++)
    {
        int index = it->first;
        RS_SCENE scene = it->second;

        RECT_S rect = main_pos[index].first;
        int level = main_pos[index].second;
        main_vo_->StartChannel(index, rect, level);

        if (scene == TEA_FEA || scene == STU_FEA || scene == MAIN)
        {
            if (scene == MAIN)
            {
                MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(main_screen_, 4, 12, index);
            }
            else
            {
                MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(scene, 4, 12, index);
            }
        }
        else
        {
            vpss_arr_[scene]->StartUserChannel(3, rect);
            MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(scene, 3, 12, index);
        }
    }

    CONFIG->scene_ = scene_conf;
    CONFIG->WriteToFile();
    main_screen_started_ = true;
}

void VideoManager::CloseMainScreen()
{
    if (!init_ && !main_screen_started_)
        return;

    for (auto it = CONFIG->scene_.mapping.begin(); it != CONFIG->scene_.mapping.end(); it++)
    {
        int index = it->first;
        RS_SCENE scene = it->second;

        if (scene == TEA_FEA || scene == STU_FEA || scene == MAIN)
        {
            if (scene == MAIN)
            {
                MPPSystem::UnBind<HI_ID_VPSS, HI_ID_VOU>(main_screen_, 4, 12, index);
            }
            else
            {
                MPPSystem::UnBind<HI_ID_VPSS, HI_ID_VOU>(scene, 4, 12, index);
            }
        }
        else
        {
            vpss_arr_[scene]->StopUserChannal(3);
            MPPSystem::UnBind<HI_ID_VPSS, HI_ID_VOU>(scene, 3, 12, index);
        }

        main_vo_->StopChannel(index);
    }

    main_screen_started_ = false;
}

void VideoManager::StartDisplayScreen(const Config::Display &display_conf)
{
    if (!init_ && display_screen_started_)
        return;

    display_vo_->Initialize({0, VO_INTF_VGA | VO_INTF_HDMI, display_conf.disp_vo_intf_sync});

    std::map<int, RECT_S> display_pos = display_conf.display_pos;
    for (auto it = display_conf.mapping.begin(); it != display_conf.mapping.end(); it++)
    {
        int index = it->first;
        RS_SCENE scene = it->second;

        RECT_S rect = display_pos[index];
        main_vo_->StartChannel(index, rect, 0);
        MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(scene, 2, 0, index);
    }

    CONFIG->display_ = display_conf;
    CONFIG->WriteToFile();
    display_screen_started_ = true;
}

void VideoManager::CloseDisplayScreen()
{
    if (!init_ && !display_screen_started_)
        return;

    for (auto it = CONFIG->display_.mapping.begin(); it != CONFIG->display_.mapping.end(); it++)
    {
        int index = it->first;
        RS_SCENE scene = it->second;

        MPPSystem::UnBind<HI_ID_VPSS, HI_ID_VOU>(scene, 2, 0, index);
        main_vo_->StopChannel(index);
    }

    display_vo_->Close();
    display_screen_started_ = false;
}

void VideoManager::StartVideoEncode(const Config::Video &video_conf)
{
    if (!init_ && encode_stared_)
        return;

    if (!CONFIG->IsResourceMode())
    {
        for (int i = 0; i < 8; i++)
        {
            if (i == 7)
            {
                venc_arr_[i]->Initialize({i, i, video_conf.normal_record_width, video_conf.normal_record_height, 25, 25, 0, video_conf.normal_record_bitrate, VENC_RC_MODE_H264CBR, true});
                vpss_arr_[6]->StartUserChannel(3, {0, 0, (HI_U32)video_conf.normal_record_width, (HI_U32)video_conf.normal_record_height});
                MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(6, 3, i, 0);
            }
            else
            {
                venc_arr_[i]->Initialize({i, i, video_conf.normal_live_width, video_conf.normal_live_height, 25, 25, 0, video_conf.normal_live_bitrate, VENC_RC_MODE_H264CBR, true});
                vpss_arr_[i]->StartUserChannel(1, {0, 0, (HI_U32)video_conf.normal_live_width, (HI_U32)video_conf.normal_live_height});
                MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(i, 1, i, 0);
            }
        }
    }
    else
    {
        for (int i = 0; i < 7; i++)
        {
            venc_arr_[i]->Initialize({i, i, video_conf.res_width, video_conf.res_height, 25, 25, 0, video_conf.res_bitrate, VENC_RC_MODE_H264CBR, true});
            vpss_arr_[i]->StartUserChannel(1, {0, 0, (HI_U32)video_conf.res_width, (HI_U32)video_conf.res_height});
            MPPSystem::Bind<HI_ID_VPSS, HI_ID_GROUP>(i, 1, i, 0);
        }
    }

    CONFIG->video_ = video_conf;
    CONFIG->WriteToFile();

    encode_stared_ = true;
}

void VideoManager::CloseVideoEncode()
{
    if (!init_ && !encode_stared_)
        return;

    if (!CONFIG->IsResourceMode())
    {
        for (int i = 0; i < 8; i++)
        {
            if (i == 7)
            {
                MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(6, 3, i, 0);
                vpss_arr_[6]->StopUserChannal(3);
                venc_arr_[i]->Close();
            }
            else
            {
                MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(i, 1, i, 0);
                vpss_arr_[i]->StopUserChannal(1);
                venc_arr_[i]->Close();
            }
        }
    }
    else
    {
        for (int i = 0; i < 7; i++)
        {
            MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(i, 1, i, 0);
            vpss_arr_[i]->StopUserChannal(1);
            venc_arr_[i]->Close();
        }
    }

    encode_stared_ = false;
}

void VideoManager::ChangeMainScreen(RS_SCENE new_main_screen)
{
    if (!main_screen_started_)
        return;

    for (auto it = CONFIG->scene_.mapping.begin(); it != CONFIG->scene_.mapping.end(); it++)
    {
        int index = it->first;
        RS_SCENE scene = it->second;
        if (scene == MAIN)
        {
            MPPSystem::UnBind<HI_ID_VPSS, HI_ID_VOU>(main_screen_, 4, 12, index);
            MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(new_main_screen, 4, 12, index);
            main_screen_ = new_main_screen;
        }
    }
}

void VideoManager::OnSwitchEvent(RS_SCENE scene)
{
    if (!init_ || scene == main_screen_)
        return;

    if (CONFIG->scene_.mode == Config::Scene::Mode::PIP_MODE)
    {
        if (CONFIG->scene_.mapping[1] == scene || pip_changed_)
        {
            Config::Scene scene_conf;
            scene_conf.mode = CONFIG->scene_.mode;
            scene_conf.mapping[0] = CONFIG->scene_.mapping[1];
            scene_conf.mapping[1] = CONFIG->scene_.mapping[0];

            CloseMainScreen();
            StartMainScreen(scene_conf);

            pip_changed_ = !pip_changed_;
            main_screen_ = scene;
        }
    }
    else
    {
        pip_changed_ = false;
        ChangeMainScreen(scene);
    }
}

void VideoManager::ChangePCCaputreMode(Config::Adv7842 adv7842)
{
    if (!init_)
        return;
    if (adv7842.pc_capture_mode == adv7842.pc_capture_mode)
        return;
    sig_detect_->SetPCCaptureMode(adv7842.pc_capture_mode);
    CONFIG->adv7842_ = adv7842;
    CONFIG->WriteToFile();
}
} // namespace rs