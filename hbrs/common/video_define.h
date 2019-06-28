#pragma once

enum CaptureMode
{
    CAPTURE_MODE_720P,
    CAPTURE_MODE_1080P
};

namespace rs
{
template <typename FrameT>
class VideoSink
{
public:
    virtual ~VideoSink() {}
    virtual void OnFrame(const FrameT &) {}
    virtual void OnFrame(const FrameT &, int32_t chn) {}
};

struct VideoInputFormat
{
    bool has_signal;
    int width;
    int height;
    bool interlaced;
    int frame_rate;

    bool operator!=(const VideoInputFormat &other)
    {
        if ((has_signal != other.has_signal) ||
            (width != other.width) ||
            (height != other.height) ||
            (interlaced != other.interlaced))
            return true;
        return false;
    }
};

struct VIFmtListener
{
public:
    virtual ~VIFmtListener() {}
    virtual void OnChange(const VideoInputFormat &fmt) = 0;
    virtual void OnStop() = 0;
};
} // namespace rs

#define VPSS_MAIN_SCREEN_GRP 10
#define VPSS_ENCODE_CHN 1
#define VPSS_PIP_CHN 2

#define VO_HD_DEV 0
#define VO_CVBS_DEV 3
#define VO_VIR_DEV 10

#define VO_VIR_MODE CAPTURE_MODE_1080P
#define PCIV_WINDOW_SIZE 7340032