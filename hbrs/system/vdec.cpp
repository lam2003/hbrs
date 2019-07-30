#include "system/vdec.h"
#include "common/err_code.h"

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

    VDEC_CHN_ATTR_S chn_attr;
    memset(&chn_attr, 0, sizeof(chn_attr));

    chn_attr.enType = PT_H264;
    chn_attr.u32BufSize = params_.width * params_.height * 1.5;
    chn_attr.u32Priority = 1;
    chn_attr.u32PicWidth = params_.width;
    chn_attr.u32PicHeight = params_.height;
    chn_attr.stVdecVideoAttr.enMode = VIDEO_MODE_STREAM;
    chn_attr.stVdecVideoAttr.u32RefFrameNum = 1;
    chn_attr.stVdecVideoAttr.s32SupportBFrame = 0;

    ret = HI_MPI_VDEC_CreateChn(params_.chn, &chn_attr);
    if (ret != KSuccess)
    {
        log_e("HI_MPI_VDEC_CreateChn failed with %#x", ret);
        return KSDKError;
    }

    VDEC_CHN_PARAM_S chn_params;
    memset(&chn_params, 0, sizeof(chn_params));
    ret = HI_MPI_VDEC_GetChnParam(params_.chn, &chn_params);
    if (ret != KSuccess)
    {
        log_e("HI_MPI_VDEC_GetChnParam failed with %#x", ret);
        return KSDKError;
    }

    chn_params.s32ChanStrmOFThr = 1;
    chn_params.s32ChanErrThr = 0;
    ret = HI_MPI_VDEC_SetChnParam(params_.chn, &chn_params);
    if (ret != KSuccess)
    {
        log_e("HI_MPI_VDEC_SetChnParam failed with %#x", ret);
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

    if (params_.chn == chn)
        HI_MPI_VDEC_SendStream(params_.chn, const_cast<VDEC_STREAM_S *>(&st), HI_IO_NOBLOCK);
}

} // namespace rs