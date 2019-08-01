#include "live/rtmp_live.h"
#include "common/err_code.h"

namespace rs
{

using namespace rtmp;

RTMPLive::RTMPLive() : run_(false),
                       thread_(nullptr),
                       init_(false)
{
}

RTMPLive::~RTMPLive()
{
    Close();
}

int RTMPLive::Initialize(const Params &params)
{
    if (init_)
        return KInitialized;

    params_ = params;

    run_ = true;
    thread_ = std::unique_ptr<std::thread>(new std::thread([this]() {
        RTMPStreamer streamer;

        Frame frame;
        MMZBuffer mmz_buffer(2 * 1024 * 1024);

        bool init = false;

        while (run_)
        {
            int ret;

            if (!init)
            {
                ret = streamer.Initialize(params_.url);
                if (ret != KSuccess)
                {
                    int wait_sec = 10; //5秒后发起重连
                    while (run_ && wait_sec--)
                        usleep(500000); //500ms
                }
                else
                {
                    mux_.lock();
                    buffer_.Clear();
                    mux_.unlock();
                    init = true;
                }
            }

            if (init)
            {
                {
                    std::unique_lock<std::mutex> lock(mux_);
                    if (buffer_.Get(reinterpret_cast<uint8_t *>(&frame), sizeof(frame)))
                    {

                        if (frame.type == Frame::AUDIO)
                        {
                            memcpy(mmz_buffer.vir_addr, buffer_.GetCurrentPos(), frame.data.aframe.len);
                            frame.data.aframe.data = mmz_buffer.vir_addr;
                            buffer_.Consume(frame.data.aframe.len);
                        }
                        else
                        {
                            memcpy(mmz_buffer.vir_addr, buffer_.GetCurrentPos(), frame.data.vframe.len);
                            frame.data.vframe.data = mmz_buffer.vir_addr;
                            buffer_.Consume(frame.data.vframe.len);
                        }
                    }
                    else if (run_)
                    {
                        cond_.wait(lock);
                        continue;
                    }
                }

                if (frame.type == Frame::AUDIO)
                {
                    ret = streamer.WriteAudioFrame(frame.data.aframe);
                    if (ret != KSuccess)
                    {
                        log_w("disconnect....,try to connect after 5 sec");
                        streamer.Close();
                        init = false; //断开连接,尝试重连
                    }
                }
                else
                {
                    ret = streamer.WriteVideoFrame(frame.data.vframe);
                    if (ret != KSuccess)
                    {
                        log_w("disconnect....,try to connect after 5 sec");
                        streamer.Close();
                        init = false; //断开连接,尝试重连
                    }
                }
            }
        }

        streamer.Close();
    }));

    init_ = true;
    return KSuccess;
}

void RTMPLive::Close()
{
    if (!init_)
        return;

    run_ = false;
    cond_.notify_all();
    thread_->join();
    thread_.reset();
    thread_ = nullptr;

    init_ = false;
}

void RTMPLive::OnFrame(const VENCFrame &video_frame)
{
    if (!init_)
        return;
    mux_.lock();

    if (buffer_.FreeSpace() < sizeof(Frame) + video_frame.len)
    {
        mux_.unlock();
        log_e("[%s]buffer fill", params_.url.c_str());
        return;
    }

    Frame frame;
    frame.type = Frame::VIDEO;
    frame.data.vframe = video_frame;

    buffer_.Append(reinterpret_cast<uint8_t *>(&frame), sizeof(frame));
    buffer_.Append(video_frame.data, video_frame.len);

    cond_.notify_one();
    mux_.unlock();
}

void RTMPLive::OnFrame(const AENCFrame &audio_frame)
{
    if (!init_)
        return;
    mux_.lock();

    if (buffer_.FreeSpace() < sizeof(Frame) + audio_frame.len)
    {
        mux_.unlock();
        log_e("[%s]buffer fill", params_.url.c_str());
        return;
    }

    Frame frame;
    frame.type = Frame::AUDIO;
    frame.data.aframe = audio_frame;

    buffer_.Append(reinterpret_cast<uint8_t *>(&frame), sizeof(frame));
    buffer_.Append(audio_frame.data, audio_frame.len);

    cond_.notify_one();
    mux_.unlock();
}

} // namespace rs