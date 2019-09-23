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

    operator Json::Value() const;
    static bool IsOk(const Json::Value &root);
    Params &operator=(const Json::Value &root);
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
    Buffer<allocator_2048k> buffer_;

    std::atomic<bool> run_;
    std::unique_ptr<std::thread> thread_;
    bool init_;
};
} // namespace rs