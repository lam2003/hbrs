#pragma once

#include "live/normal_rtmp_streamer.h"
#include "live/srs_rtmp_streamer.h"
#include "common/buffer.h"

namespace rs
{

namespace rtmp
{
struct Params
{
    std::string url;
    bool has_audio;
    bool only_try_once;

    operator Json::Value() const;
    static bool IsOk(const Json::Value &root);
    Params &operator=(const Json::Value &root);
};
} // namespace rtmp

class RTMPLive : public AudioSink<AENCFrame>, public VideoSink<VENCFrame>
{
public:
    explicit RTMPLive();

    virtual ~RTMPLive();

    int Initialize(const rtmp::Params &params, bool use_srs = false);

    void Close();

    void OnFrame(const AENCFrame &frame) override;

    void OnFrame(const VENCFrame &frame) override;

protected:
    void HandleVideoOnly();

    template <typename T>
    void HandleAV();

private:
    rtmp::Params params_;
    std::mutex mux_;
    std::condition_variable cond_;
    Buffer<allocator_2048k> buffer_;
    std::atomic<bool> run_;
    std::unique_ptr<std::thread> thread_;
    bool use_srs_;
    bool init_;
};
} // namespace rs