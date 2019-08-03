#pragma once

#include "global.h"
#include "common/av_define.h"
#include "common/buffer.h"

namespace rs
{

class AudioEncode : public AudioSink<AIFrame>
{
public:
    explicit AudioEncode();

    virtual ~AudioEncode();

    int Initialize();

    void Close();

    void AddAudioSink(AudioSink<AENCFrame> *sink);

    void RemoveAudioSink(AudioSink<AENCFrame> *sink);

    void OnFrame(const AIFrame &frame) override;

private:
    std::mutex sinks_mux_;
    std::vector<AudioSink<AENCFrame> *> sinks_;
    std::mutex mux_;
    std::condition_variable cond_;
    Buffer<allocator_512k> buffer_;
    std::atomic<bool> run_;
    std::unique_ptr<std::thread> thread_;
    bool init_;
};
} // namespace rs