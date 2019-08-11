#pragma once

//self
#include "global.h"
#include "common/av_define.h"

namespace rs
{

namespace venc
{
struct Params
{
    int32_t grp;
    int32_t chn;
    int32_t width;
    int32_t height;
    int32_t src_frame_rate;
    int32_t dst_frame_rate;
    int32_t profile;
    int32_t bitrate;
    VENC_RC_MODE_E mode;
    bool set_ts;
};

} // namespace venc

class VideoEncode
{
public:
    explicit VideoEncode();

    virtual ~VideoEncode();

    int32_t Initialize(const venc::Params &params);

    void Close();

    void AddVideoSink(std::shared_ptr<VideoSink<VENCFrame>> sink);

    void RemoveVideoSink(std::shared_ptr<VideoSink<VENCFrame>> sink);

private:
    venc::Params params_;
    std::mutex sinks_mux_;
    std::vector<std::shared_ptr<VideoSink<VENCFrame>>> sinks_;
    std::unique_ptr<std::thread> thread_;
    std::atomic<bool> run_;
    bool init_;
};
} // namespace rs