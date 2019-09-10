#pragma once

#include "common/config.h"
#include "common/switch.h"
#include "system/sig_detect.h"

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
class FrameBuffer;
class OsdTs;
class Osd;

class AVManager : public SwitchEventListener, public std::enable_shared_from_this<AVManager>, public SignalStatusListener
{
public:
    explicit AVManager();

    virtual ~AVManager();

    int Initialize();

    void Close(const std::string &opt = "");

    void StartLocalLive(const Config::LocalLive &local_lives);

    void CloseLocalLive();

    void StartRemoteLive(const Config::RemoteLive &remote_live);

    void CloseRemoteLive();

    void StartRecord(const Config::Record &records);

    void CloseRecord();

    void StartMainScreen(const Config::Scene &scene_conf);

    void CloseMainScreen();

    void StartDisplayScreen(const Config::Display &display_conf);

    void CloseDisplayScreen();

    void StartSmallDisplayScreen();

    void CloseSmallDisplayScreen();

    void StartVideoEncode(const Config::Video &video_conf);

    void CloseVideoEncode();

    void ChangeMainScreen(RS_SCENE scene);

    void OnSwitchEvent(RS_SCENE scene) override;

    void ChangePCCaputreMode(Config::Adv7842 adv7842);

    void OnUpdate(const std::vector<VideoInputFormat> &fmts) override;

    void StartOsdTs();

    void CloseOsdTs();

    void StartOsd(const Config::Osd &osd);

    void CloseOsd();

private:
    std::vector<std::shared_ptr<VIHelper>> vi_arr_;
    std::vector<std::shared_ptr<VideoProcess>> vpss_tmp_arr_;
    std::vector<std::shared_ptr<VideoOutput>> vo_arr_;
    std::vector<std::shared_ptr<VideoDecode>> vdec_arr_;
    std::vector<std::shared_ptr<VideoProcess>> vpss_arr_;
    std::vector<std::shared_ptr<VideoEncode>> venc_arr_;
    std::vector<std::shared_ptr<RTMPLive>> live_arr_;
    std::vector<std::shared_ptr<MP4Record>> record_arr_;
    std::vector<std::shared_ptr<Osd>> osd_arr_;

    std::shared_ptr<VideoOutput> display_vo_;
    std::shared_ptr<FrameBuffer> display_fb_;
    std::shared_ptr<VideoOutput> small_display_vo_;
    std::shared_ptr<FrameBuffer> small_display_fb_;
    std::shared_ptr<VideoOutput> main_vo_;
    std::shared_ptr<PCIVComm> pciv_comm_;
    std::shared_ptr<PCIVTrans> pciv_trans_;
    std::shared_ptr<SigDetect> sig_detect_;
    std::shared_ptr<OsdTs> osd_ts_;
    std::shared_ptr<AudioInput> ai_;
    std::shared_ptr<AudioEncode> aenc_;
    std::shared_ptr<AudioOutput> ao_;

    RS_SCENE main_screen_;

    bool encode_stared_;
    bool display_screen_started_;
    bool small_display_screen_started_;
    bool main_screen_started_;
    bool local_live_started_;
    bool remote_live_started_;
    bool record_started_;
    bool pip_changed_;
    bool osd_started_;
    bool init_;
};
} // namespace rs