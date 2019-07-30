#include "live/rtmp_live.h"
#include "common/err_code.h"

namespace rs
{

using namespace rtmp;

RtmpLive::RtmpLive()
{
}

RtmpLive::~RtmpLive()
{
    Close();
}

int RtmpLive::Initialize(const Params &params)
{
    if (init_)
        return KInitialized;

    params_ = params;

    run_ = true;
    thread_ = std::unique_ptr<std::thread>(new std::thread([this]() {
        RTMPStreamer streamer;

        Frame frame;
        MMZBuffer mmz_buffer;
        allocator_2048k::mmz_malloc(mmz_buffer);

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
                        streamer.Close();
                        init = false; //断开连接,尝试重连
                    }
                }
                else
                {
                    ret = streamer.WriteVideoFrame(frame.data.vframe);
                    if (ret != KSuccess)
                    {
                        streamer.Close();
                        init = false; //断开连接,尝试重连
                    }
                }
            }
        }

        streamer.Close();
        allocator_2048k::mmz_free(mmz_buffer);
    }));

    init_ = true;
    return KSuccess;
}

void RtmpLive::Close()
{
    if (!init_)
        return;

    init_ = false;
}

} // namespace rs