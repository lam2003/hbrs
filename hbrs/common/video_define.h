#pragma once

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
    virtual void OnChange(const VideoInputFormat &fmt){};
    virtual void OnChange(const VideoInputFormat &fmt, int chn) {}
    virtual void OnStop() = 0;
};

template <typename FrameT>
class VideoSender
{
public:
    virtual ~VideoSender() {}
    virtual int GetFrame(int chn, FrameT &frame) = 0;
    virtual int ReleaseFrame(int chn, const FrameT &frame) = 0;
    virtual int StartChannel(int chn, const SIZE_S &size) = 0;
    virtual int StopChannal(int chn) = 0;
};

template <typename FrameT>
class VideoRecver
{
public:
    virtual ~VideoRecver() {}
    virtual int SendFrame(int chn, FrameT &frame) = 0;
    virtual int StartChannel(int chn, const RECT_S &rect, int level) = 0;
    virtual int StopChannel(int chn) = 0;
};

struct MMZBuffer
{
    uint8_t *vir_addr;
    uint32_t phy_addr;
};

} // namespace rs
