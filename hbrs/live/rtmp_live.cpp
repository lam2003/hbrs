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

void RTMPLive::HandleVideoOnly()
{
    RTMPStreamer streamer;
    Frame frame;
    MMZBuffer mmz_buffer(2 * 1024 * 1024);
    uint64_t vts_base = 0;
    uint64_t ts;

    bool init = false;

    while (run_)
    {
        int ret;

        if (!init)
        {
            ret = streamer.Initialize(params_.url, params_.has_audio);
            if (ret != KSuccess)
            {
                if (params_.only_try_once)
                {
                    log_w("[%s]thread quit because set only_try_once", params_.url.c_str());
                    return;
                }
                int wait_sec = 10; //5秒后发起重连
                while (run_ && wait_sec--)
                    usleep(500000); //500ms
            }
            else
            {
                vts_base = 0;
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
                    if (vts_base == 0)
                        vts_base = frame.data.vframe.ts;
                    ts = frame.data.vframe.ts - vts_base;
                    frame.data.vframe.ts = ts / 1000;

                    memcpy(mmz_buffer.vir_addr, buffer_.GetCurrentPos(), frame.data.vframe.len);
                    frame.data.vframe.data = mmz_buffer.vir_addr;
                    buffer_.Consume(frame.data.vframe.len);
                }
                else if (run_)
                {
                    cond_.wait(lock);
                    continue;
                }
            }

            ret = streamer.WriteVideoFrame(frame.data.vframe);
            if (ret != KSuccess)
            {
                log_w("disconnect....,try to connect after 5 sec");
                streamer.Close();
                init = false; //断开连接,尝试重连
            }
        }
    }

    streamer.Close();
}

void RTMPLive::HandleAV()
{
    RTMPStreamer streamer;
    Frame frame;
    std::multimap<uint64_t, std::pair<Frame, std::shared_ptr<uint8_t>>> frms;
    uint32_t nb_videos, nb_audios;
    uint64_t ats_base, vts_base;
    uint64_t ts;

    bool init = false;

    while (run_)
    {
        int ret;

        if (!init)
        {
            ret = streamer.Initialize(params_.url, params_.has_audio);
            if (ret != KSuccess)
            {
                if (params_.only_try_once)
                {
                    log_w("[%s]thread quit because set only_try_once", params_.url.c_str());
                    return;
                }
                int wait_sec = 10; //5秒后发起重连
                while (run_ && wait_sec--)
                    usleep(500000); //500ms
            }
            else
            {
                nb_videos = 0;
                nb_audios = 0;
                vts_base = 0;
                ats_base = 0;
                frms.clear();
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
                        if (ats_base == 0)
                            ats_base = frame.data.aframe.ts;
                        ts = frame.data.aframe.ts - ats_base;
                        frame.data.aframe.ts = ts / 1000;

                        std::shared_ptr<uint8_t> data(new uint8_t[frame.data.aframe.len](), std::default_delete<uint8_t[]>());
                        memcpy(data.get(), buffer_.GetCurrentPos(), frame.data.aframe.len);
                        frms.insert(std::make_pair(frame.data.aframe.ts, std::make_pair(frame, data)));
                        nb_audios++;
                        buffer_.Consume(frame.data.aframe.len);
                    }
                    else
                    {
                        if (vts_base == 0)
                            vts_base = frame.data.vframe.ts;
                        ts = frame.data.vframe.ts - vts_base;
                        frame.data.vframe.ts = ts / 1000;

                        std::shared_ptr<uint8_t> data(new uint8_t[frame.data.vframe.len](), std::default_delete<uint8_t[]>());
                        memcpy(data.get(), buffer_.GetCurrentPos(), frame.data.vframe.len);
                        frms.insert(std::make_pair(frame.data.vframe.ts, std::make_pair(frame, data)));
                        nb_videos++;
                        buffer_.Consume(frame.data.vframe.len);
                    }

                    if (nb_videos >= 50 || nb_audios >= 50)
                    {
                        log_w("[%s]-->nb_videos:%d,nb_audios:%d,clear buffer", params_.url.c_str(), nb_videos, nb_audios);
                        nb_videos = 0;
                        nb_audios = 0;
                        frms.clear();
                        continue;
                    }
                }
                else if (run_)
                {
                    cond_.wait(lock);
                    continue;
                }
            }

            while (nb_audios > 1 && nb_videos > 1)
            {
                std::multimap<uint64_t, std::pair<Frame, std::shared_ptr<uint8_t>>>::iterator it = frms.begin();
                if (it->second.first.type == Frame::AUDIO)
                {
                    it->second.first.data.aframe.data = it->second.second.get();
                    ret = streamer.WriteAudioFrame(it->second.first.data.aframe);
                    if (ret != KSuccess)
                    {
                        log_w("send failed,disconnect....,try to connect after 5 sec");
                        streamer.Close();
                        init = false;

                        int wait_sec = 10; //5秒后发起重连
                        while (run_ && wait_sec--)
                            usleep(500000); //500ms
                    }
                    nb_audios--;
                }
                else
                {
                    it->second.first.data.vframe.data = it->second.second.get();
                    ret = streamer.WriteVideoFrame(it->second.first.data.vframe);
                    if (ret != KSuccess)
                    {
                        log_w("send failed,disconnect....,try to connect after 5 sec");
                        streamer.Close();
                        init = false;

                        int wait_sec = 10; //5秒后发起重连
                        while (run_ && wait_sec--)
                            usleep(500000); //500ms
                    }
                    nb_videos--;
                }
                frms.erase(it);
            }
        }
    }

    streamer.Close();
}

int RTMPLive::Initialize(const Params &params)
{
    if (init_)
        return KInitialized;

    params_ = params;

    run_ = true;
    thread_ = std::unique_ptr<std::thread>(new std::thread([this]() {
        if (params_.has_audio)
        {
            HandleAV();
        }
        else
        {
            HandleVideoOnly();
        }
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
        // log_e("[%s]buffer fill", params_.url.c_str());
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
        // log_e("[%s]buffer fill", params_.url.c_str());
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