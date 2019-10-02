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
#include "system/mpp.h"
#include "system/fb.h"
#include "common/err_code.h"
#include "common/utils.h"
#include "common/json.h"
#include "common/http_client.h"
#include "model/http_notify_req.h"
#include "osd/osd_ts.h"
#include "osd/osd.h"

namespace rs
{

AVManager::AVManager() : display_vo_(nullptr),
                         display_fb_(nullptr),
                         small_display_vo_(nullptr),
                         small_display_fb_(nullptr),
                         main_vo_(nullptr),
                         pciv_comm_(nullptr),
                         pciv_trans_(nullptr),
                         sig_detect_(nullptr),
                         osd_ts_(nullptr),
                         ai_(nullptr),
                         aenc_(nullptr),
                         ao_(nullptr),
                         main_screen_(TEA_FEA),
                         encode_stared_(false),
                         display_screen_started_(false),
                         main_screen_started_(false),
                         local_live_started_(false),
                         remote_live_started_(false),
                         record_started_(false),
                         pip_changed_(false),
                         osd_started_(false),
                         init_(false)
{
}
AVManager::~AVManager()
{
}
int AVManager::Initialize()
{
    if (init_)
        return KInitialized;

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

    live_arr_.resize(8);
    for (int i = 0; i < 8; i++)
        live_arr_[i] = std::make_shared<RTMPLive>();

    record_arr_.resize(8);
    for (int i = 0; i < 8; i++)
        record_arr_[i] = std::make_shared<MP4Record>();

    display_vo_ = std::make_shared<VideoOutput>();

    display_fb_ = std::make_shared<FrameBuffer>();

    small_display_vo_ = std::make_shared<VideoOutput>();

    small_display_fb_ = std::make_shared<FrameBuffer>();

    main_vo_ = std::make_shared<VideoOutput>();

    pciv_comm_ = std::make_shared<PCIVComm>();

    pciv_trans_ = std::make_shared<PCIVTrans>();

    sig_detect_ = std::make_shared<SigDetect>();

    osd_ts_ = std::make_shared<OsdTs>();

    ai_ = std::make_shared<AudioInput>();

    ao_ = std::make_shared<AudioOutput>();

    aenc_ = std::make_shared<AudioEncode>();
#if 0
    for (int i = 0; i < 2; i++)
        vi_arr_[i]->Start(RS_MAX_WIDTH, RS_MAX_HEIGHT, 30, false); //预设摄像头采集时序1080P30FPS
#endif
    for (int i = 0; i < 2; i++)
    {
        vpss_tmp_arr_[i]->Initialize({10 + i});
        vpss_tmp_arr_[i]->StartUserChannel(1, {0, 0, RS_MAX_WIDTH, RS_MAX_HEIGHT});
    }

    for (int i = 0; i < 2; i++)
    {
        vo_arr_[i]->Initialize({10 + i, 0, VO_OUTPUT_1080P25});
        vo_arr_[i]->StartChannel(0, {0, 0, RS_MAX_WIDTH, RS_MAX_HEIGHT}, 0);
        vi_arr_[i]->SetVideoOutput(vo_arr_[i]);
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
    sig_detect_->SetSignalStatusListener(shared_from_this());
    sig_detect_->Initialize(pciv_comm_, CONFIG->adv7842_.pc_capture_mode);

    ai_->Initialize({4, 0});

    ao_->Initialize({4, 0});

    aenc_->Initialize();

    ai_->AddAudioSink(aenc_);

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

    MPPSystem::Bind<HI_ID_AI, HI_ID_AO>(4, 0, 4, 0);

    init_ = true;

    StartMainScreen(CONFIG->scene_);

    StartDisplayScreen(CONFIG->display_);

    StartSmallDisplayScreen();

    StartVideoEncode(CONFIG->video_);

    StartOsdTs();

    StartOsd(CONFIG->osd_);

    return KSuccess;
}

void AVManager::Close(const std::string &opt)
{
    if (!init_)
        return;

    CloseRecord();

    CloseLocalLive();

    CloseRemoteLive();

    CloseOsd();

    CloseOsdTs();

    CloseVideoEncode();

    CloseSmallDisplayScreen();

    CloseDisplayScreen();

    CloseMainScreen();

    MPPSystem::UnBind<HI_ID_AI, HI_ID_AO>(4, 0, 4, 0);

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

    ai_->RemoveAllAudioSink();

    aenc_->RemoveAllAudioSink();

    aenc_->Close();

    ao_->Close();

    ai_->Close();

    sig_detect_->Close();
    sig_detect_->SetSignalStatusListener(nullptr);
    sig_detect_->RemoveAllVIFmtListener();

    pciv_trans_->Close();
    pciv_trans_->RemoveAllVideoSink();

    if (opt == "shutdown")
    {
        pciv::Msg msg;
        msg.type = pciv::Msg::Type::SHUTDOWN;
        pciv_comm_->Send(RS_PCIV_SLAVE1_ID, RS_PCIV_CMD_PORT, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
        pciv_comm_->Send(RS_PCIV_SLAVE3_ID, RS_PCIV_CMD_PORT, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
    }
    else if (opt == "reboot")
    {
        pciv::Msg msg;
        msg.type = pciv::Msg::Type::REBOOT;
        pciv_comm_->Send(RS_PCIV_SLAVE1_ID, RS_PCIV_CMD_PORT, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
        pciv_comm_->Send(RS_PCIV_SLAVE3_ID, RS_PCIV_CMD_PORT, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
    }

    pciv_comm_->Close();

    main_vo_->Close();

    for (int i = 0; i < 7; i++)
        vpss_arr_[i]->Close();

    for (int i = 0; i < 4; i++)
        vdec_arr_[i]->Close();

    for (int i = 0; i < 2; i++)
    {
        vi_arr_[i]->SetVideoOutput(nullptr);
        vo_arr_[i]->Close();
    }

    for (int i = 0; i < 2; i++)
        vpss_tmp_arr_[i]->Close();

    for (int i = 0; i < 2; i++)
        vi_arr_[i]->Stop();

    aenc_.reset();
    aenc_ = nullptr;

    ao_.reset();
    ao_ = nullptr;

    ai_.reset();
    ai_ = nullptr;

    osd_ts_.reset();
    osd_ts_ = nullptr;

    sig_detect_.reset();
    sig_detect_ = nullptr;

    pciv_trans_.reset();
    pciv_trans_ = nullptr;

    pciv_comm_.reset();
    pciv_comm_ = nullptr;

    main_vo_.reset();
    main_vo_ = nullptr;

    small_display_fb_.reset();
    small_display_fb_ = nullptr;

    small_display_vo_.reset();
    small_display_vo_ = nullptr;

    display_fb_.reset();
    display_fb_ = nullptr;

    display_vo_.reset();
    display_vo_ = nullptr;

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

void AVManager::StartLocalLive(const Config::LocalLive &local_lives)
{
    if (!init_ || local_live_started_)
        return;

    for (const std::pair<RS_SCENE, rtmp::Params> &item : local_lives.lives)
    {
        RS_SCENE scene = item.first;
        rtmp::Params rtmp_params = item.second;
        live_arr_[scene]->Initialize(rtmp_params);
        venc_arr_[scene]->AddVideoSink(live_arr_[scene]);
        if (rtmp_params.has_audio)
            aenc_->AddAudioSink(live_arr_[scene]);
    }

    CONFIG->local_lives_ = local_lives;
    local_live_started_ = true;
}

void AVManager::CloseLocalLive()
{
    if (!init_ || !local_live_started_)
        return;

    for (const std::pair<RS_SCENE, rtmp::Params> &item : CONFIG->local_lives_.lives)
    {
        RS_SCENE scene = item.first;
        rtmp::Params rtmp_params = item.second;
        if (rtmp_params.has_audio)
            aenc_->RemoveAudioSink(live_arr_[scene]);
        venc_arr_[scene]->RemoveVideoSink(live_arr_[scene]);
        live_arr_[scene]->Close();
    }

    CONFIG->local_lives_.lives.clear();
    local_live_started_ = false;
}

void AVManager::StartRemoteLive(const Config::RemoteLive &remote_live)
{
    if (!init_ || remote_live_started_)
        return;

    live_arr_[MAIN2]->Initialize(remote_live.live,true);
    venc_arr_[MAIN]->AddVideoSink(live_arr_[MAIN2]);
    if (remote_live.live.has_audio)
        aenc_->AddAudioSink(live_arr_[MAIN2]);

    CONFIG->remote_live_ = remote_live;
    remote_live_started_ = true;
}

void AVManager::CloseRemoteLive()
{
    if (!init_ || !remote_live_started_)
        return;

    if (CONFIG->remote_live_.live.has_audio)
        aenc_->RemoveAudioSink(live_arr_[MAIN2]);
    venc_arr_[MAIN]->RemoveVideoSink(live_arr_[MAIN2]);
    live_arr_[MAIN2]->Close();

    CONFIG->remote_live_.live.url = "";
    remote_live_started_ = false;
}

void AVManager::StartRecord(const Config::Record &records)
{
    if (!init_ || record_started_)
        return;

    for (const std::pair<RS_SCENE, mp4::Params> &item : records.records)
    {
        RS_SCENE scene = item.first;
        mp4::Params record_params = item.second;

        record_arr_[scene]->Initialize(record_params);
        venc_arr_[scene]->AddVideoSink(record_arr_[scene]);
        aenc_->AddAudioSink(record_arr_[scene]);
    }

    CONFIG->records_ = records;
    record_started_ = true;
}

void AVManager::CloseRecord()
{
    if (!init_ || !record_started_)
        return;

    for (const std::pair<RS_SCENE, mp4::Params> &item : CONFIG->records_.records)
    {
        RS_SCENE scene = item.first;

        aenc_->RemoveAudioSink(record_arr_[scene]);
        venc_arr_[scene]->RemoveVideoSink(record_arr_[scene]);
        record_arr_[scene]->Close();
    }

    CONFIG->records_.records.clear();
    record_started_ = false;
}

void AVManager::StartMainScreen(const Config::Scene &scene_conf)
{
    if (!init_ || main_screen_started_)
        return;

    std::map<int, std::pair<RECT_S, int>> main_pos = VideoOutput::GetScenePos(scene_conf.mode);
    for (auto it = scene_conf.mapping.begin(); it != scene_conf.mapping.end(); it++)
    {
        int index = it->first;
        RS_SCENE scene = it->second;

        RECT_S rect = main_pos[index].first;
        int level = main_pos[index].second;
        main_vo_->StartChannel(index, rect, level);

        if (scene == MAIN)
        {
            MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(main_screen_, 4, 12, index);
        }
        else if (scene == TEA_FEA || scene == STU_FEA)
        {
            MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(scene, 4, 12, index);
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

void AVManager::CloseMainScreen()
{
    if (!init_ || !main_screen_started_)
        return;

    for (auto it = CONFIG->scene_.mapping.begin(); it != CONFIG->scene_.mapping.end(); it++)
    {
        int index = it->first;
        RS_SCENE scene = it->second;

        if (scene == MAIN)
        {
            MPPSystem::UnBind<HI_ID_VPSS, HI_ID_VOU>(main_screen_, 4, 12, index);
        }
        else if (scene == TEA_FEA || scene == STU_FEA)
        {
            MPPSystem::UnBind<HI_ID_VPSS, HI_ID_VOU>(scene, 4, 12, index);
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

void AVManager::StartDisplayScreen(const Config::Display &display_conf)
{
    if (!init_ || display_screen_started_)
        return;

    display_vo_->Initialize({0, VO_INTF_VGA | VO_INTF_HDMI, display_conf.disp_vo_intf_sync});
    display_fb_->Initialize({"/dev/fb0", display_conf.disp_vo_intf_sync});

    std::map<int, RECT_S> display_pos = display_conf.display_pos;
    for (auto it = display_conf.mapping.begin(); it != display_conf.mapping.end(); it++)
    {
        int index = it->first;
        RS_SCENE scene = it->second;
        RECT_S rect = display_pos[index];
        display_vo_->StartChannel(index, rect, 0);
        MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(scene, 2, 0, index);
    }

    CONFIG->display_ = display_conf;
    CONFIG->WriteToFile();
    display_screen_started_ = true;
}

void AVManager::CloseDisplayScreen()
{
    if (!init_ || !display_screen_started_)
        return;

    for (auto it = CONFIG->display_.mapping.begin(); it != CONFIG->display_.mapping.end(); it++)
    {
        int index = it->first;
        RS_SCENE scene = it->second;

        MPPSystem::UnBind<HI_ID_VPSS, HI_ID_VOU>(scene, 2, 0, index);
        display_vo_->StopChannel(index);
    }

    display_fb_->Close();
    display_vo_->Close();
    display_screen_started_ = false;
}

void AVManager::StartSmallDisplayScreen()
{
    if (!init_ || small_display_screen_started_)
        return;

    small_display_vo_->Initialize({3, VO_INTF_CVBS, VO_OUTPUT_PAL});
    small_display_fb_->Initialize({"/dev/fb3", VO_OUTPUT_PAL});

    small_display_screen_started_ = true;
}

void AVManager::CloseSmallDisplayScreen()
{
    if (!init_ || !small_display_screen_started_)
        return;

    small_display_fb_->Close();
    small_display_vo_->Close();

    small_display_screen_started_ = false;
}

void AVManager::StartVideoEncode(const Config::Video &video_conf)
{
    if (!init_ || encode_stared_)
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

void AVManager::CloseVideoEncode()
{
    if (!init_ || !encode_stared_)
        return;

    if (!CONFIG->IsResourceMode())
    {
        for (int i = 0; i < 8; i++)
        {
            if (i == 7)
            {
                MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(6, 3, i, 0);
                vpss_arr_[6]->StopUserChannal(3);
            }
            else
            {
                MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(i, 1, i, 0);
                vpss_arr_[i]->StopUserChannal(1);
            }
            venc_arr_[i]->Close();
            venc_arr_[i]->RemoveAllVideoSink();
        }
    }
    else
    {
        for (int i = 0; i < 7; i++)
        {
            MPPSystem::UnBind<HI_ID_VPSS, HI_ID_GROUP>(i, 1, i, 0);
            vpss_arr_[i]->StopUserChannal(1);
            venc_arr_[i]->Close();
            venc_arr_[i]->RemoveAllVideoSink();
        }
    }

    encode_stared_ = false;
}

void AVManager::ChangeMainScreen(RS_SCENE new_main_screen)
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

void AVManager::OnSwitchEvent(RS_SCENE scene)
{
    if (!init_)
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

void AVManager::ChangePCCaputreMode(Config::Adv7842 adv7842)
{
    if (!init_)
        return;
    sig_detect_->SetPCCaptureMode(adv7842.pc_capture_mode);
    CONFIG->adv7842_ = adv7842;
    CONFIG->WriteToFile();
}

void AVManager::OnUpdate(const std::vector<VideoInputFormat> &fmts)
{
    Json::Value root;

    for (size_t i = 0; i < fmts.size(); i++)
    {
        Json::Value item = fmts[i];
        root[std::to_string(i)] = item;
    }

    std::string msg = JsonUtils::toStr(root);

    HttpNotifyReq req;
    req.type = HttpNotifyReq::Type::SIGNAL_STATUS;
    req.msg = msg;

    std::string data = JsonUtils::toStr(req);
    HttpClient::Instance()->Post(data);
}

void AVManager::StartOsdTs()
{
    if (!init_)
        return;
    if (CONFIG->osd_ts_.add_ts)
    {
        osd_ts_->Initialize({0,
                             6,
                             CONFIG->osd_ts_.font_file,
                             CONFIG->osd_ts_.font_size,
                             CONFIG->osd_ts_.color_r,
                             CONFIG->osd_ts_.color_g,
                             CONFIG->osd_ts_.color_b,
                             CONFIG->osd_ts_.x,
                             CONFIG->osd_ts_.y,
                             CONFIG->osd_ts_.time_format});
    }
}

void AVManager::CloseOsdTs()
{
    if (!init_)
        return;
    osd_ts_->Close();
}

void AVManager::StartOsd(const Config::Osd &osd)
{
    if (!init_ || osd_started_)
        return;

    int i = 1;
    for (const Config::Osd::Item &item : osd.items)
    {
        std::shared_ptr<Osd> osd = std::make_shared<Osd>();
        osd->Initialize({i,
                         6,
                         item.font_file,
                         item.font_size,
                         item.color_r,
                         item.color_g,
                         item.color_b,
                         item.x,
                         item.y,
                         item.content});
        osd_arr_.push_back(osd);
        i++;
    }

    osd_started_ = true;
}

void AVManager::CloseOsd()
{
    if (!init_ || !osd_started_)
        return;

    for (size_t i = 0; i < osd_arr_.size(); i++)
        osd_arr_[i]->Close();
    osd_arr_.clear();
    osd_started_ = false;
}

} // namespace rs