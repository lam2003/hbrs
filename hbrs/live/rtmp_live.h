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
    bool has_audio;

    operator Json::Value() const
    {
        Json::Value root;
        root["url"] = url;
        root["has_audio"] = has_audio;
        return root;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!root.isMember("url") ||
            !root["url"].isString())
            return false;
        return true;
    }

    Params &operator=(const Json::Value &root)
    {
        url = root["url"].asString();
        return *this;
    }
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

protected:
    void HandleVideoOnly();

    void HandleAV();

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