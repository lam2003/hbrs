#pragma once

#include "live/rtmp_streamer.h"
#include "common/buffer.h"

namespace rs
{

namespace rtmp
{
struct Params
{
    std::string url;
};
} // namespace rtmp

class RTMPLive : public AudioSink<AENCFrame>, public VideoSink<VENCFrame>
{
public:
    explicit RTMPLive();

    virtual ~RTMPLive();

    int Initialize(const rtmp::Params &params);

    void Close();

    void OnFrame(const AENCFrame &frame) override;

    void OnFrame(const VENCFrame &frame) override;

private:
    rtmp::Params params_;
    std::mutex mux_;
    std::condition_variable cond_;
    Buffer<allocator_2048k> buffer_;

    std::atomic<bool> run_;
    std::unique_ptr<std::thread> thread_;
    bool init_;
};
} // namespace rs