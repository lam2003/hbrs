#pragma once

#include "common/buffer.h"
#include "common/av_define.h"
#include "common/av_define.h"

namespace rs
{
namespace mp4_muxer
{
struct Params
{
    int width;
    int height;
    int frame_rate;
    int samplate_rate;
    std::string filename;
};
} // namespace mp4_muxer

class MP4Muxer
{
public:
    explicit MP4Muxer();

    virtual ~MP4Muxer();

    int Initialize(const mp4_muxer::Params &params);

    void Close();

    int WriteVideoFrame(const VENCFrame &frame);

    int WriteAudioFrame(const AENCFrame &frame);

private:
    mp4_muxer::Params params_;
    MMZBuffer mmz_bufer_;
    int64_t aduration_;
    std::string sps_;
    std::string pps_;
    std::string sei_;
    uint64_t vts_base;
    uint64_t ats_base;
    AVFormatContext *ctx_;
    bool init_;
};
} // namespace rs