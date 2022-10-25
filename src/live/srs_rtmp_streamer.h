#pragma once

#include "global.h"
#include "common/av_define.h"

namespace rs
{

class SRSRtmpStreamer
{
public:
    explicit SRSRtmpStreamer();

    virtual ~SRSRtmpStreamer();

    int Initialize(const std::string &url, bool has_audio);

    void Close();

    int WriteVideoFrame(const VENCFrame &frame);

    int WriteAudioFrame(const AENCFrame &frame);

private:
    std::string url_;
    srs_rtmp_t rtmp_;
    bool init_;

    static const int DefaultTimeOut;
};
} // namespace rs