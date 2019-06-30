//stl
#include <thread>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
//self
#include "common/global.h"

#pragma once

namespace rs
{

namespace vdec
{
struct Params
{
    int32_t chn;
    int width;
    int height;
};

} // namespace vdec

class VideoDecode : public VideoSink<VDEC_STREAM_S>
{
public:
    explicit VideoDecode();

    virtual ~VideoDecode();

    int32_t Initialize(const vdec::Params &params);

    void Close();

    void OnFrame(const VDEC_STREAM_S &st, int chn) override;

    void AddVideoSink(VideoSink<VIDEO_FRAME_INFO_S> *sink);

    void RemoveAllVideoSink();

private:
    vdec::Params params_;
    std::mutex mux_;
    std::vector<VideoSink<VIDEO_FRAME_INFO_S> *> sinks_;
    std::atomic<bool> run_;
    std::unique_ptr<std::thread> thread_;
    bool init_;
};
} // namespace rs