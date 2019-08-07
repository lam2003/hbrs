#pragma once

#include "global.h"

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

class VideoManager
{
public:
    explicit VideoManager();

    virtual ~VideoManager();
    
    int Initialize();

    void Close();

protected:
    void StartVideoEncode();

    void StopVideoEncode();

private:
    std::vector<std::shared_ptr<VIHelper>> vi_arr_;
    std::vector<std::shared_ptr<VideoOutput>> vo_arr_;
    std::vector<std::shared_ptr<VideoDecode>> vdec_arr_;
    std::vector<std::shared_ptr<VideoProcess>> vpss_arr_;
    std::vector<std::shared_ptr<VideoEncode>> venc_arr_;
    std::shared_ptr<VideoOutput> main_vo_;
    std::shared_ptr<VideoOutput> display_vo_;

    std::shared_ptr<PCIVComm> pciv_comm_;
    std::shared_ptr<PCIVTrans> pciv_trans_;
    std::shared_ptr<SigDetect> sig_detect_;

    std::shared_ptr<RTMPLive> live_arr_;
    std::shared_ptr<MP4Record> record_arr_;

    bool init_;
};
} // namespace rs