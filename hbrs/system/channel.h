#pragma once

#include "global.h"
#include "common/video_define.h"
#include "common/err_code.h"

namespace rs
{


class Channel
{
public:
    explicit Channel(VideoSender<VIDEO_FRAME_INFO_S> *sender, VideoRecver<VIDEO_FRAME_INFO_S> *recver) : src_chn_(-1),
                                                                                 dst_chn_(-1),
                                                                                 sender_(sender),
                                                                                 recver_(recver),
                                                                                 run_(false),
                                                                                 thread_(nullptr),
                                                                                 init_(false) {}

    virtual ~Channel()
    {
        Close();
    }

    int Initialize(int src_chn, int dst_chn, const RECT_S &rect, int level)
    {
        if (init_)
            return KInitialized;
        int ret;

        src_chn_ = src_chn;
        dst_chn_ = dst_chn;

        ret = sender_->StartChannel(src_chn_, {rect.u32Width, rect.u32Height});
        if (ret != KSuccess)
            return ret;

        ret = recver_->StartChannel(dst_chn_, rect, level);
        if (ret != KSuccess)
            return ret;

        run_ = true;
        thread_ = std::unique_ptr<std::thread>(new std::thread([this]() {
            int ret;
            VIDEO_FRAME_INFO_S frame;
            while (run_)
            {
                ret = sender_->GetFrame(src_chn_, frame);
                if (ret != KSuccess && ret != KTimeout)
                    return;

                if (ret == KTimeout)
                {
                    usleep(10000); //10ms
                    continue;
                }

                ret = recver_->SendFrame(dst_chn_, frame);
                if (ret != KSuccess)
                    return;

                ret = sender_->ReleaseFrame(src_chn_, frame);
                if (ret != KSuccess)
                    return;
            }
        }));

        init_ = true;
        return KSuccess;
    }

    void Close()
    {
        if (!init_)
            return;
        run_ = false;
        thread_->join();
        thread_.reset();
        thread_ = nullptr;

        sender_->StopChannal(src_chn_);
        sender_->StopChannal(dst_chn_);

        src_chn_ = -1;
        dst_chn_ = -1;

        sender_ = nullptr;
        recver_ = nullptr;
        init_ = false;
    }

private:
    int src_chn_;
    int dst_chn_;
    VideoSender<VIDEO_FRAME_INFO_S> *sender_;
    VideoRecver<VIDEO_FRAME_INFO_S> *recver_;
    std::atomic<bool> run_;
    std::unique_ptr<std::thread> thread_;
    bool init_;
};
} // namespace rs