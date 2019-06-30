#include "system/vdec.h"

namespace rs
{
VideoDecode::VideoDecode() : init_(false)
{
}

VideoDecode::~VideoDecode()
{
    Close();
}

int VideoDecode::Initialize(const vdec::Params &params)
{
    if (init_)
        return KInitialized;

    int ret;

    params_ = params;

    VDEC_CHN_ATTR_S attr;
    memset(&attr, 0, sizeof(attr));

    attr.enType = PT_H264;
    attr.u32BufSize = params_.width * params_.height * 2;
    attr.u32Priority = 1;
    attr.u32PicWidth = params_.width;
    attr.u32PicHeight = params_.height;
    attr.stVdecVideoAttr.enMode = VIDEO_MODE_STREAM;
    attr.stVdecVideoAttr.u32RefFrameNum = 2;
    attr.stVdecVideoAttr.s32SupportBFrame = 0;

    ret = HI_MPI_VDEC_CreateChn(params_.chn, &attr);
    if (ret != KSuccess)
    {
        log_e("HI_MPI_VDEC_CreateChn failed with %#", ret);
        return KSDKError;
    }

    run_ = true;
    thread_ = std::unique_ptr<std::thread>(new std::thread([this]() {
        int ret;

        ret = HI_MPI_VDEC_StartRecvStream(params_.chn);
        if (ret != KSuccess)
        {
            log_e("HI_MPI_VDEC_CreateChn failed with %#x", ret);
            return;
        }

        VIDEO_FRAME_INFO_S frame;
        while (run_)
        {
            ret = HI_MPI_VDEC_GetImage_TimeOut(params_.chn, &frame, 500); //500ms
            if (ret != KSuccess)
            {
                if (ret == HI_ERR_VDEC_BUSY)
                    continue;

                log_e("HI_MPI_VDEC_GetImage_TimeOut failed with %#x", ret);
                return;
            }
            {
                std::unique_lock<std::mutex> lock(mux_);
                for (size_t i = 0; i < sinks_.size(); i++)
                    sinks_[i]->OnFrame(frame);
            }
            HI_MPI_VDEC_ReleaseImage(params_.chn, &frame);
        }

        ret = HI_MPI_VDEC_StopRecvStream(params_.chn);
        if (ret != KSuccess)
            log_e("HI_MPI_VDEC_StopRecvStream failed with %#x", ret);
    }));

    init_ = true;

    return KSuccess;
}

void VideoDecode::Close()
{
    if (!init_)
        return;

    int ret;

    run_ = false;
    thread_->join();
    thread_.reset();
    thread_ = nullptr;
    sinks_.clear();

    ret = HI_MPI_VDEC_DestroyChn(params_.chn);
    if (ret != KSuccess)
        log_e("HI_MPI_VDEC_DestroyChn failed with %#x", ret);

    init_ = false;
}

void VideoDecode::OnFrame(const VDEC_STREAM_S &st, int chn)
{
    if (!init_)
        return;

    int ret;

    if (params_.chn == chn)
    {
        ret = HI_MPI_VDEC_SendStream(params_.chn, const_cast<VDEC_STREAM_S *>(&st), HI_IO_NOBLOCK);
        if (ret != KSuccess)
        {
            log_e("HI_MPI_VDEC_SendStream failed with %#x", ret);
            return;
        }
    }
}

void VideoDecode::AddVideoSink(VideoSink<VIDEO_FRAME_INFO_S> *sink)
{
    std::unique_lock<std::mutex> lock(mux_);
    sinks_.push_back(sink);
}

void VideoDecode::RemoveAllVideoSink()
{
    std::unique_lock<std::mutex> lock(mux_);
    sinks_.clear();
}
} // namespace rs