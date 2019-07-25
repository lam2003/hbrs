#include "system/aenc.h"
#include "common/err_code.h"

#include <aacenc.h>

namespace rs
{
AudioEncode::AudioEncode() : run_(false),
                             thread_(nullptr),
                             init_(false)
{
}

AudioEncode::~AudioEncode()
{
    Close();
}

int AudioEncode::Initialize()
{
    if (init_)
        return KInitialized;

    run_ = true;
    thread_ = std::unique_ptr<std::thread>(new std::thread([this]() {
        int ret;
        uint64_t ts = 0;
        uint64_t duration = (1000000 * 1024) / 44100;

        AACENC_CONFIG config;
        ret = AACInitDefaultConfig(&config);
        if (ret != KSuccess)
        {
            log_e("AACInitDefaultConfig failed");
            return;
        }

        config.coderFormat = AACLC;
        config.bitRate = 32000;
        config.bitsPerSample = 16;
        config.sampleRate = 44100;
        config.bandWidth = 44100 / 2;
        config.nChannelsIn = 2;
        config.nChannelsOut = 2;
        config.quality = AU_QualityExcellent;

        AAC_ENCODER_S *encoder;
        ret = AACEncoderOpen(&encoder, &config);
        if (ret)
        {
            log_e("AACEncoderOpen failed");
            return;
        }

        {
            //清空缓存
            std::unique_lock<std::mutex> lock(mux_);
            buf_.Clear();
        }

        uint8_t tmp_buf[4096];
        uint8_t out_buf[4096];
        int out_len;
        while (run_)
        {
            {
                std::unique_lock<std::mutex> lock(mux_);
                if (!buf_.Get(tmp_buf, sizeof(tmp_buf)))
                {
                    if (run_)
                    {
                        cond_.wait(lock);
                        continue;
                    }
                }
            }

            if (run_)
            {
                ret = AACEncoderFrame(encoder, reinterpret_cast<short *>(tmp_buf), out_buf, &out_len);
                if (ret != KSuccess)
                {
                    log_e("AACEncoderFrame failed with %#x", ret);
                    return;
                }

                AENCFrame aenc_frame;
                aenc_frame.data = out_buf;
                aenc_frame.len = out_len;
                aenc_frame.ts = ts;

                {
                    std::unique_lock<std::mutex> lock(sinks_mux_);
                    for (size_t i = 0; i < sinks_.size(); i++)
                        sinks_[i]->OnFrame(aenc_frame);
                }

                ts += duration;
            }
        }

        AACEncoderClose(encoder);
    }));

    init_ = true;
    return KSuccess;
}

void AudioEncode::Close()
{
    if (!init_)
        return;

    run_ = false;
    cond_.notify_all();
    thread_->join();
    thread_.reset();
    thread_ = nullptr;
    sinks_.clear();
    init_ = false;
}

void AudioEncode::OnFrame(const AIFrame &frame)
{
    if (!init_)
        return;

    std::unique_lock<std::mutex> lock(mux_);
    if (!buf_.Append(frame.data, frame.len))
    {
        log_e("append data to buffer failed");
        return;
    }
    cond_.notify_one();
}

void AudioEncode::AddAudioSink(AudioSink<AENCFrame> *sink)
{
    std::unique_lock<std::mutex> lock(sinks_mux_);
    sinks_.push_back(sink);
}

void AudioEncode::RemoveAllAudioSink()
{
    std::unique_lock<std::mutex> lock(sinks_mux_);
    sinks_.clear();
}
} // namespace rs
