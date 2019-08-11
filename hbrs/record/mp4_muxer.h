#pragma once

#include "common/buffer.h"
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

protected:
    int InitContext();

private:
    MMZBuffer mmz_buffer_;
    MP4FileHandle hdl_;
    MP4TrackId vtrack_;
    MP4TrackId atrack_;
    int width_;
    int height_;
    int frame_rate_;
    int samplate_rate_;
    std::string filename_;
    std::string sps_;
    std::string pps_;
    std::string sei_;
    std::atomic<bool> init_ctx_;

    bool init_;
};
} // namespace rs