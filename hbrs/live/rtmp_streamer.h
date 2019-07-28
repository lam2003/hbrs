#pragma once

#include "global.h"
#include "common/av_define.h"

namespace rs
{
namespace rtmp
{
struct Params
{
    std::string url;
};
} // namespace rtmp
class RTMPStreamer
{
public:
    explicit RTMPStreamer();

    virtual ~RTMPStreamer();

    int Initialize(const rtmp::Params &params);

    void Close();

    int WriteVideoFrame(const VENCFrame &frame);

    int WriteAudioFrame(const AENCFrame &frame);

private:
    rtmp::Params params_;
    srs_rtmp_t rtmp_;
    bool init_;

    static const int DefaultTimeOut;
};
} // namespace rs