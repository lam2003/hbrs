#pragma once

#include "global.h"
#include "common/av_define.h"

namespace rs
{

namespace ai
{
struct Params
{
    int dev;
    int chn;
};
} // namespace ai
class AudioInput
{
public:
    explicit AudioInput();

    virtual ~AudioInput();

    int Initialize(const ai::Params &params);

    void Close();

    void AddAudioSink(std::shared_ptr<AudioSink<AIFrame>> sink);

    void RemoveAllAudioSink();

private:
    ai::Params params_;
    std::mutex mux_;
    std::vector<std::shared_ptr<AudioSink<AIFrame>>> sinks_;
    std::unique_ptr<std::thread> thread_;
    std::atomic<bool> run_;
    bool init_;

    static const int BufferLen;
};
} // namespace rs