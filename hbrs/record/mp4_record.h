#pragma once

#include "record/mp4_muxer.h"

namespace rs
{

namespace mp4
{
struct Params
{
    int width;
    int height;
    int frame_rate;
    int samplate_rate;
    std::string filename;
    int segment_duration;
    bool need_to_segment;

    operator Json::Value() const
    {
        Json::Value root;
        root["width"] = width;
        root["height"] = height;
        root["frame_rate"] = frame_rate;
        root["samplate_rate"] = samplate_rate;
        root["filename"] = filename;
        root["segment_duration"] = segment_duration;
        root["need_to_segment"] = need_to_segment;
        return root;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!root.isMember("filename") ||
            !root["filename"].isString() ||
            !root.isMember("segment_duration") ||
            !root["segment_duration"].isInt() ||
            !root.isMember("need_to_segment") ||
            !root["need_to_segment"].isBool())
            return false;
        return true;
    }

    Params &operator=(const Json::Value &root)
    {
        filename = root["filename"].asString();
        segment_duration = root["segment_duration"].asInt();
        need_to_segment = root["need_to_segment"].asBool();
        return *this;
    }

};
} // namespace mp4

class MP4Record : public AudioSink<AENCFrame>, public VideoSink<VENCFrame>
{
public:
    explicit MP4Record();

    virtual ~MP4Record();

    int Initialize(const mp4::Params &params);

    void Close();

    void OnFrame(const AENCFrame &frame) override;

    void OnFrame(const VENCFrame &frame) override;

private:
    mp4::Params params_;
    std::mutex mux_;
    std::condition_variable cond_;
    Buffer<allocator_2048k> buffer_;

    std::atomic<bool> run_;
    std::unique_ptr<std::thread> thread_;
    bool init_;
};
} // namespace rs