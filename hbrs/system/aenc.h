#pragma once

#include "global.h"
#include "common/audio_define.h"
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

    void RemoveAllAudioSink();

    void OnFrame(const AIFrame &frame) override;

private:
    std::mutex sinks_mux_;
    std::vector<AudioSink<AENCFrame> *> sinks_;
    std::mutex mux_;
    std::condition_variable cond_;
    Buffer<allocator_16k> buf_;
    std::atomic<bool> run_;
    std::unique_ptr<std::thread> thread_;
    bool init_;
};
} // namespace rs