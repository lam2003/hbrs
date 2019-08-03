#pragma once

#include "global.h"
#include "common/av_define.h"

namespace rs
{

class RTMPStreamer
{
public:
    explicit RTMPStreamer();

    virtual ~RTMPStreamer();

    int Initialize(const std::string &url);

    void Close();

    int WriteVideoFrame(const VENCFrame &frame);

    int WriteAudioFrame(const AENCFrame &frame);

private:
    srs_rtmp_t rtmp_;
    bool init_;

    static const int DefaultTimeOut;
};
} // namespace rs