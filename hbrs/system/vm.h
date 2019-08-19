#pragma once

#include "common/config.h"

namespace rs
{

class VIHelper;
class VideoOutput;
class VideoProcess;
class VideoDecode;
class VideoEncode;
class PCIVComm;
class PCIVTrans;
class SigDetect;
class RTMPLive;
class MP4Record;
class AudioInput;
class AudioEncode;
class AudioOutput;

class VideoManager
{
public:
    explicit VideoManager();

    virtual ~VideoManager();

    int Initialize();

    void Close();

    void StartLocalLive(const Config::LocalLive &local_lives);

    void CloseLocalLive();

    void StartRemoteLive(const Config::RemoteLive &remote_live);

    void CloseRemoteLive();

    void StartResourceRecord(const Config::ResourceRecord &records);

    void CloseResourceRecord();

    void StartNormalRecord(const Config::NormalRecord &record);

    void CloseNormalRecord();

    void StartMainScreen(const Config::Scene &scene_conf);

    void CloseMainScreen();

    void StartDisplayScreen(const Config::Display &display_conf);

    void CloseDisplayScreen();

    void StartVideoEncode(const Config::Video &video_conf);

    void CloseVideoEncode();

private:
    std::vector<std::shared_ptr<VIHelper>> vi_arr_;
    std::vector<std::shared_ptr<VideoProcess>> vpss_tmp_arr_;
    std::vector<std::shared_ptr<VideoOutput>> vo_arr_;
    std::vector<std::shared_ptr<VideoDecode>> vdec_arr_;
    std::vector<std::shared_ptr<VideoProcess>> vpss_arr_;
    std::vector<std::shared_ptr<VideoEncode>> venc_arr_;
    std::vector<std::shared_ptr<RTMPLive>> live_arr_;
    std::vector<std::shared_ptr<MP4Record>> record_arr_;

    std::shared_ptr<VideoOutput> display_vo_;
    std::shared_ptr<VideoOutput> main_vo_;
    std::shared_ptr<AudioInput> ai_;
    std::shared_ptr<AudioEncode> aenc_;
    std::shared_ptr<AudioOutput> ao_;
    std::shared_ptr<PCIVComm> pciv_comm_;
    std::shared_ptr<PCIVTrans> pciv_trans_;
    std::shared_ptr<SigDetect> sig_detect_;

    RS_SCENE main_screen_;

    bool encode_stared_;
    bool display_screen_started_;
    bool main_screen_started_;
    bool local_live_started_;
    bool remote_live_started_;
    bool resource_record_started_;
    bool normal_record_started_;
    bool init_;
};
} // namespace rs