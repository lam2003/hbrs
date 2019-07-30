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

class RtmpLive
{
public:
    explicit RtmpLive();

    virtual ~RtmpLive();

    int Initialize(const rtmp::Params &params);

    void Close();

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