#include "system/ao.h"
#include "common/err_code.h"

namespace rs
{
AudioOutput::AudioOutput() : init_(false)
{
}

AudioOutput::~AudioOutput()
{
    Close();
}

int AudioOutput::Initialize(const ao::Params &params)
{
    if (init_)
        return KInitialized;

    int ret;

    params_ = params;

    AIO_ATTR_S attr;
    memset(&attr, 0, sizeof(attr));
    attr.enSamplerate = AUDIO_SAMPLE_RATE_48000;
    attr.enBitwidth = AUDIO_BIT_WIDTH_16;
    attr.enWorkmode = AIO_MODE_I2S_SLAVE;
    attr.u32FrmNum = 30;
    attr.u32PtNumPerFrm = 1024;
    attr.u32ChnCnt = 2;
    attr.u32ClkSel = 1;
    attr.enSoundmode = AUDIO_SOUND_MODE_STEREO;

    ret = HI_MPI_AO_SetPubAttr(params_.dev, &attr);
    if (ret != KSuccess)
    {
        log_e("HI_MPI_AO_SetPubAttr failed with %#x", ret);
        return KSDKError;
    }

    ret = HI_MPI_AO_Enable(params_.dev);
    if (ret != KSuccess)
    {
        log_e("HI_MPI_AO_Enable failed with %#x", ret);
        return KSDKError;
    }

    ret = HI_MPI_AO_EnableChn(params_.dev, params_.chn);
    if (ret != KSuccess)
    {
        log_e("HI_MPI_AO_EnableChn failed with %#x", ret);
        return KSDKError;
    }

    init_ = true;

    return KSuccess;
}

void AudioOutput::Close()
{
    if (!init_)
        return;
    int ret;
    ret = HI_MPI_AO_DisableChn(params_.dev, params_.chn);
    if (ret != KSuccess)
        log_e("HI_MPI_AO_DisableChn failed with %#x", ret);
    ret = HI_MPI_AO_Disable(params_.dev);
    if (ret != KSuccess)
        log_e("HI_MPI_AO_Disable failed with %#x", ret);
    init_ = false;
}
} // namespace rs