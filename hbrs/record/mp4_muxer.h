#pragma once

#include "common/buffer.h"
#include "common/av_define.h"
#include "common/av_define.h"

namespace rs
{

class MP4Muxer
{
public:
    explicit MP4Muxer();

    virtual ~MP4Muxer();

    int Initialize(int width, int height, int frame_rate, int samplate_rate, const std::string filename);

    void Close();

    int WriteVideoFrame(const VENCFrame &frame);

    int WriteAudioFrame(const AENCFrame &frame);

private:
    MMZBuffer mmz_bufer_;
    std::string sps_;
    std::string pps_;
    std::string sei_;
    uint64_t vts_base;
    uint64_t ats_base;
    AVFormatContext *ctx_;
    bool init_;
};
} // namespace rs