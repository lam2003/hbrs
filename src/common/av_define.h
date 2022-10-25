#pragma once

namespace rs
{
template <typename FrameT>
class VideoSink
{
public:
    virtual ~VideoSink() {}
    virtual void OnFrame(const FrameT &) {}
    virtual void OnFrame(const FrameT &, int32_t) {}
};

struct VENCFrame
{
    uint64_t ts;
    uint32_t len;
    uint8_t *data;
    H264E_NALU_TYPE_E type;
};

template <typename FrameT>
class AudioSink
{
public:
    virtual ~AudioSink() {}
    virtual void OnFrame(const FrameT &) {}
    virtual void OnFrame(uint8_t *data, uint32_t len) {}
};

struct AIFrame
{
    uint64_t ts;
    uint32_t len;
    uint8_t *data;
};

struct AENCFrame
{
    uint64_t ts;
    uint32_t len;
    uint8_t *data;
};

struct Frame
{
    enum Type
    {
        AUDIO = 0,
        VIDEO
    };

    Type type;
    union Data {
        AENCFrame aframe;
        VENCFrame vframe;
    };
    Data data;
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

    operator Json::Value() const
    {
        Json::Value root;
        root["has_signal"] = has_signal;
        root["width"] = width;
        root["height"] = height;
        root["interlaced"] = interlaced;
        root["frame_rate"] = frame_rate;
        return root;
    }
};

struct VIFmtListener
{
public:
    virtual ~VIFmtListener() {}
    virtual void OnChange(const VideoInputFormat &fmt){};
    virtual void OnChange(const VideoInputFormat &fmt, int chn) {}
};

} // namespace rs
