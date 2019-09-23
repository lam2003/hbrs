#include "system/aenc.h"
#include "common/err_code.h"

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
    log_d("AENC start");
    run_ = true;
    thread_ = std::unique_ptr<std::thread>(new std::thread([this]() {
        int ret;

        AACENC_CONFIG config;
        ret = AACInitDefaultConfig(&config);
        if (ret != KSuccess)
        {
            log_e("AACInitDefaultConfig failed");
            return;
        }

        config.coderFormat = AACLC;
        config.bitRate = 128000;
        config.bitsPerSample = 16;
        config.sampleRate = 48000;
        config.nChannelsIn = 2;
        config.nChannelsOut = 2;
        config.quality = AU_QualityLow;

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
            buffer_.Clear();
        }

        // AIFrame frame;
        uint8_t tmp_buf[4096];
        uint8_t out_buf[4096];
        int out_len;
        while (run_)
        {
            {
                std::unique_lock<std::mutex> lock(mux_);
                // if (buffer_.Get(reinterpret_cast<uint8_t *>(&frame), sizeof(frame)))
                // {
                //     memcpy(tmp_buf, buffer_.GetCurrentPos(), frame.len);
                //     frame.data = tmp_buf;
                //     buffer_.Consume(frame.len);
                // }
                if (buffer_.Get(tmp_buf, sizeof(tmp_buf)))
                {
                }
                else if (run_)
                {
                    cond_.wait(lock);
                    continue;
                }
                else
                {
                    continue;
                }
            }

            // ret = AACEncoderFrame(encoder, reinterpret_cast<short *>(frame.data), out_buf, &out_len);
            AACEncoderFrame(encoder, reinterpret_cast<short *>(tmp_buf), out_buf, &out_len);
            if (ret != KSuccess)
            {
                log_e("AACEncoderFrame failed with %#x", ret);
                return;
            }

            AENCFrame aenc_frame;
            aenc_frame.data = out_buf;
            aenc_frame.len = out_len;
            HI_MPI_SYS_GetCurPts(&aenc_frame.ts);
            // aenc_frame.ts = frame.ts;

            {
                std::unique_lock<std::mutex> lock(sinks_mux_);
                for (size_t i = 0; i < sinks_.size(); i++)
                    sinks_[i]->OnFrame(aenc_frame);
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

    log_d("AENC stop");
    run_ = false;
    cond_.notify_all();
    thread_->join();
    thread_.reset();
    thread_ = nullptr;
    sinks_.clear();
    init_ = false;
}

// void AudioEncode::OnFrame(const AIFrame &frame)
void AudioEncode::OnFrame(uint8_t *data, uint32_t len)
{
    if (!init_)
        return;

    std::unique_lock<std::mutex> lock(mux_);
    // if (buffer_.FreeSpace() < frame.len + sizeof(frame))

    if (buffer_.FreeSpace() < len)
    {
        log_e("append data to buffer failed");
        return;
    }

    // buffer_.Append(const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(&frame)), sizeof(frame));
    // buffer_.Append(frame.data, frame.len);
    buffer_.Append(data, len);
    cond_.notify_one();
}

void AudioEncode::AddAudioSink(std::shared_ptr<AudioSink<AENCFrame>> sink)
{
    std::unique_lock<std::mutex> lock(sinks_mux_);
    sinks_.push_back(sink);
}

void AudioEncode::RemoveAudioSink(std::shared_ptr<AudioSink<AENCFrame>> sink)
{
    std::unique_lock<std::mutex> lock(sinks_mux_);

    for (auto it = sinks_.begin(); it != sinks_.end(); it++)
    {
        if (it->get() == sink.get())
        {
            sinks_.erase(it);
            break;
        }
    }
}

void AudioEncode::RemoveAllAudioSink()
{
    std::unique_lock<std::mutex> lock(sinks_mux_);
    sinks_.clear();
}
} // namespace rs
