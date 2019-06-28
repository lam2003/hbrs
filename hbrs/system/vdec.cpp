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

    ret = HI_MPI_VDEC_StartRecvStream(params_.chn);
    if (ret != KSuccess)
    {
        log_e("HI_MPI_VDEC_CreateChn failed with %#x", ret);
        return KSDKError;
    }

    init_ = true;

    return KSuccess;
}

void VideoDecode::Close()
{
    if (!init_)
        return;

    int ret;

    ret = HI_MPI_VDEC_StopRecvStream(params_.chn);
    if (ret != KSuccess)
        log_e("HI_MPI_VDEC_StopRecvStream failed with %#x", ret);

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
} // namespace rs