#pragma once

namespace rs
{
template <typename FrameT>
class AudioSink
{
public:
    virtual ~AudioSink() {}
    virtual void OnFrame(const FrameT &frame) {}
};

struct AIFrame
{
    uint32_t len;
    uint8_t *data;
};

struct AENCFrame
{
    uint64_t ts;
    uint32_t len;
    uint8_t *data;
};
} // namespace rs