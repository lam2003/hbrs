#pragma once

//self
#include "global.h"
#include "common/video_define.h"

namespace rs
{

namespace vpss
{
struct Params
{
    int32_t grp;
};
} // namespace vpss

class VideoProcess : public VideoSender<VIDEO_FRAME_INFO_S>, public VideoSink<VIDEO_FRAME_INFO_S>
{

public:
    explicit VideoProcess();

    virtual ~VideoProcess();

    int32_t Initialize(const vpss::Params &params);

    void Close();

    void OnFrame(const VIDEO_FRAME_INFO_S &frame) override;

    int GetFrame(int chn, VIDEO_FRAME_INFO_S &frame) override;

    int ReleaseFrame(int chn, const VIDEO_FRAME_INFO_S &frame) override;

    int StartChannel(int chn, const SIZE_S &size) override;

    int StopChannal(int chn) override;

private:
    vpss::Params params_;
    bool init_;
}; // namespace rs
} // namespace rs