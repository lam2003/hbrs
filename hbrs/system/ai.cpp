#include "system/ai.h"
#include "common/err_code.h"

namespace rs
{

using namespace ai;

const int AudioInput::BufferLen = 4 * 1024;

static int StartTlv320(AIO_MODE_E mode, AUDIO_SAMPLE_RATE_E samplerate)
{
    int ret;

    int samplerate_val;
    int vol = 0x100;
    Audio_Ctrl audio_ctrl;
    bool pcm = false;
    bool master = true;
    bool pcm_std = false;
    bool is_44100hz_series = false;

    if (AUDIO_SAMPLE_RATE_8000 == samplerate)
    {
        samplerate_val = AC31_SET_8K_SAMPLERATE;
    }
    else if (AUDIO_SAMPLE_RATE_12000 == samplerate)
    {
        samplerate_val = AC31_SET_12K_SAMPLERATE;
    }
    else if (AUDIO_SAMPLE_RATE_11025 == samplerate)
    {
        is_44100hz_series = true;
        samplerate_val = AC31_SET_11_025K_SAMPLERATE;
    }
    else if (AUDIO_SAMPLE_RATE_16000 == samplerate)
    {
        samplerate_val = AC31_SET_16K_SAMPLERATE;
    }
    else if (AUDIO_SAMPLE_RATE_22050 == samplerate)
    {
        is_44100hz_series = true;
        samplerate_val = AC31_SET_22_05K_SAMPLERATE;
    }
    else if (AUDIO_SAMPLE_RATE_24000 == samplerate)
    {
        samplerate_val = AC31_SET_24K_SAMPLERATE;
    }
    else if (AUDIO_SAMPLE_RATE_32000 == samplerate)
    {
        samplerate_val = AC31_SET_32K_SAMPLERATE;
    }
    else if (AUDIO_SAMPLE_RATE_44100 == samplerate)
    {
        is_44100hz_series = true;
        samplerate_val = AC31_SET_44_1K_SAMPLERATE;
    }
    else if (AUDIO_SAMPLE_RATE_48000 == samplerate)
    {
        samplerate_val = AC31_SET_48K_SAMPLERATE;
    }
    else
    {
        RS_ASSERT(0);
    }

    if (AIO_MODE_I2S_MASTER == mode)
    {
        pcm = false;
        master = false;
    }
    else if (AIO_MODE_I2S_SLAVE == mode)
    {
        pcm = false;
        master = true;
    }
    else if ((AIO_MODE_PCM_MASTER_NSTD == mode) || (AIO_MODE_PCM_MASTER_STD == mode))
    {
        pcm = true;
        master = false;
    }
    else if ((AIO_MODE_PCM_SLAVE_NSTD == mode) || (AIO_MODE_PCM_SLAVE_STD == mode))
    {
        pcm = true;
        master = true;
    }
    else
    {
        RS_ASSERT(0);
    }

    const char *dev = "/dev/tlv320aic31";
    int fd = open(dev, O_RDWR);
    if (fd < 0)
    {
        log_e("open %s failed,%s", dev, strerror(errno));
        return KSystemError;
    }

    audio_ctrl.chip_num = 0;
    ret = ioctl(fd, SOFT_RESET, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl SOFT_RESET failed,%s", strerror(errno));
        return KSystemError;
    }

    audio_ctrl.ctrl_mode = master;
    audio_ctrl.if_44100hz_series = is_44100hz_series;
    audio_ctrl.sample = samplerate_val;
    ret = ioctl(fd, SET_CTRL_MODE, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl SET_CTRL_MODE failed,%s", strerror(errno));
        return KSystemError;
    }

    audio_ctrl.trans_mode = pcm;
    ret = ioctl(fd, SET_TRANSFER_MODE, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl SET_TRANSFER_MODE failed,%s", strerror(errno));
        return KSystemError;
    }

    ret = ioctl(fd, SET_DAC_SAMPLE, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl SET_DAC_SAMPLE failed,%s", strerror(errno));
        return KSystemError;
    }

    ret = ioctl(fd, SET_ADC_SAMPLE, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl SET_ADC_SAMPLE failed,%s", strerror(errno));
        return KSystemError;
    }

    audio_ctrl.if_mute_route = 0;
    audio_ctrl.input_level = 0;
    ret = ioctl(fd, LEFT_DAC_VOL_CTRL, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl LEFT_DAC_VOL_CTRL failed,%s", strerror(errno));
        return KSystemError;
    }
    ret = ioctl(fd, RIGHT_DAC_VOL_CTRL, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl RIGHT_DAC_VOL_CTRL failed,%s", strerror(errno));
        return KSystemError;
    }

    audio_ctrl.if_powerup = 1;
    ret = ioctl(fd, LEFT_DAC_POWER_SETUP, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl LEFT_DAC_POWER_SETUP failed,%s", strerror(errno));
        return KSystemError;
    }
    ret = ioctl(fd, RIGHT_DAC_POWER_SETUP, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl RIGHT_DAC_POWER_SETUP failed,%s", strerror(errno));
        return KSystemError;
    }

    if ((AIO_MODE_PCM_MASTER_STD == mode) || (AIO_MODE_PCM_SLAVE_STD == mode))
    {
        pcm_std = true;
        audio_ctrl.data_offset = pcm_std;
        ret = ioctl(fd, SET_SERIAL_DATA_OFFSET, &audio_ctrl);
        if (ret != KSuccess)
        {
            log_e("ioctl SET_SERIAL_DATA_OFFSET failed,%s", strerror(errno));
            return KSystemError;
        }
    }
    else if ((AIO_MODE_PCM_MASTER_NSTD == mode) || (AIO_MODE_PCM_SLAVE_NSTD == mode))
    {
        pcm_std = false;
        audio_ctrl.data_offset = pcm_std;
        ret = ioctl(fd, SET_SERIAL_DATA_OFFSET, &audio_ctrl);
        if (ret != KSuccess)
        {
            log_e("ioctl SET_SERIAL_DATA_OFFSET failed,%s", strerror(errno));
            return KSystemError;
        }
    }

    audio_ctrl.data_length = 0;
    ret = ioctl(fd, SET_DATA_LENGTH, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl SET_DATA_LENGTH failed,%s", strerror(errno));
        return KSystemError;
    }

    audio_ctrl.if_mute_route = 1;
    audio_ctrl.input_level = vol;
    ret = ioctl(fd, DACL1_2_LEFT_LOP_VOL_CTRL, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl DACL1_2_LEFT_LOP_VOL_CTRL failed,%s", strerror(errno));
        return KSystemError;
    }
    ret = ioctl(fd, DACR1_2_RIGHT_LOP_VOL_CTRL, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl DACR1_2_RIGHT_LOP_VOL_CTRL failed,%s", strerror(errno));
        return KSystemError;
    }

    audio_ctrl.if_mute_route = 1;
    audio_ctrl.if_powerup = 1;
    audio_ctrl.input_level = 0;
    ret = ioctl(fd, LEFT_LOP_OUTPUT_LEVEL_CTRL, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl LEFT_LOP_OUTPUT_LEVEL_CTRL failed,%s", strerror(errno));
        return KSystemError;
    }
    ret = ioctl(fd, RIGHT_LOP_OUTPUT_LEVEL_CTRL, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl RIGHT_LOP_OUTPUT_LEVEL_CTRL failed,%s", strerror(errno));
        return KSystemError;
    }

    audio_ctrl.if_mute_route = 0;
    audio_ctrl.input_level = 0;
    ret = ioctl(fd, LEFT_ADC_PGA_CTRL, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl LEFT_ADC_PGA_CTRL failed,%s", strerror(errno));
        return KSystemError;
    }
    ret = ioctl(fd, RIGHT_ADC_PGA_CTRL, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl RIGHT_ADC_PGA_CTRL failed,%s", strerror(errno));
        return KSystemError;
    }

    audio_ctrl.input_level = 0;
    ret = ioctl(fd, IN2LR_2_LEFT_ADC_CTRL, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl IN2LR_2_LEFT_ADC_CTRL failed,%s", strerror(errno));
        return KSystemError;
    }
    ret = ioctl(fd, IN2LR_2_RIGTH_ADC_CTRL, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl IN2LR_2_RIGTH_ADC_CTRL failed,%s", strerror(errno));
        return KSystemError;
    }

    audio_ctrl.if_mute_route = 1;
    audio_ctrl.input_level = 9;
    audio_ctrl.if_powerup = 1;
    ret = ioctl(fd, HPLOUT_OUTPUT_LEVEL_CTRL, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl HPLOUT_OUTPUT_LEVEL_CTRL failed,%s", strerror(errno));
        return KSystemError;
    }
    ret = ioctl(fd, HPROUT_OUTPUT_LEVEL_CTRL, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl HPROUT_OUTPUT_LEVEL_CTRL failed,%s", strerror(errno));
        return KSystemError;
    }

    close(fd);
    log_d("set aic31 ok: master = %d, mode = %d, samplerate = %d", master, mode, samplerate);
    return KSuccess;
}

int StopTlv320()
{
    Audio_Ctrl audio_ctrl;

    int ret;

    const char *dev = "/dev/tlv320aic31";
    int fd = open(dev, O_RDWR);
    if (fd < 0)
    {
        log_e("open %s failed,%s", dev, strerror(errno));
        return KSystemError;
    }

    audio_ctrl.chip_num = 0;
    ret = ioctl(fd, SOFT_RESET, &audio_ctrl);
    if (ret != KSuccess)
    {
        log_e("ioctl SOFT_RESET failed,%s", strerror(errno));
        return KSystemError;
    }
    close(fd);

    return KSuccess;
}

AudioInput::AudioInput() : init_(false)
{
}

AudioInput::~AudioInput()
{
    Close();
}

int AudioInput::Initialize(const Params &params)
{
    if (init_)
        return KInitialized;

    int ret;

    params_ = params;

    ret = StartTlv320(AIO_MODE_I2S_SLAVE, AUDIO_SAMPLE_RATE_44100);
    if (ret != KSuccess)
        return ret;

    AIO_ATTR_S attr;
    memset(&attr, 0, sizeof(attr));
    attr.enSamplerate = AUDIO_SAMPLE_RATE_44100;
    attr.enBitwidth = AUDIO_BIT_WIDTH_16;
    attr.enWorkmode = AIO_MODE_I2S_SLAVE;
    attr.u32FrmNum = 30;
    attr.u32PtNumPerFrm = 1024;
    attr.u32ChnCnt = 2;
    attr.u32ClkSel = 1;
    attr.enSoundmode = AUDIO_SOUND_MODE_STEREO;

    ret = HI_MPI_AI_SetPubAttr(params_.dev, &attr);
    if (ret != KSuccess)
    {
        log_e("HI_MPI_AI_SetPubAttr failed with %#x", ret);
        return KSDKError;
    }

    ret = HI_MPI_AI_Enable(params_.dev);
    if (ret != KSuccess)
    {
        log_e("HI_MPI_AI_Enable failed with %#x", ret);
        return KSDKError;
    }

    ret = HI_MPI_AI_EnableChn(params_.dev, params_.chn);
    if (ret != KSuccess)
    {
        log_e("HI_MPI_AI_EnableChn failed with %#x", ret);
        return KSDKError;
    }

    AI_CHN_PARAM_S param;
    memset(&param, 0, sizeof(param));
    param.u32UsrFrmDepth = 10;
    ret = HI_MPI_AI_SetChnParam(params_.dev, params_.chn, &param);
    if (ret != HI_SUCCESS)
    {
        log_e("HI_MPI_AI_SetChnParam failed with %#x", ret);
        return ret;
    }

    run_ = true;
    thread_ = std::unique_ptr<std::thread>(new std::thread([this]() {
        int ret;

        int fd = HI_MPI_AI_GetFd(params_.dev, params_.chn);
        if (fd < 0)
        {
            log_e("HI_MPI_AI_GetFd failed");
            return;
        }

        fd_set fds;
        timeval tv;
        AUDIO_FRAME_S frame;

        uint8_t *buf = reinterpret_cast<uint8_t *>(malloc(BufferLen));
        uint32_t buf_len = BufferLen;

        while (run_)
        {
            FD_ZERO(&fds);
            FD_SET(fd, &fds);

            tv.tv_sec = 0;
            tv.tv_usec = 500000; //500ms

            ret = select(fd + 1, &fds, NULL, NULL, &tv);
            if (ret > 0)
            {
                ret = HI_MPI_AI_GetFrame(params_.dev, params_.chn, &frame, NULL, HI_IO_NOBLOCK);
                if (ret != KSuccess)
                {
                    log_e("HI_MPI_AI_GetFrame failed with %#x", ret);
                    return;
                }

                if (frame.u32Len * 2 > buf_len)
                {
                    free(buf);
                    buf = reinterpret_cast<uint8_t *>(malloc(frame.u32Len * 2));
                    buf_len = frame.u32Len * 2;
                }

                uint8_t *cur_pos = buf;
                for (uint32_t i = 0; i < frame.u32Len; i += 2)
                {
                    cur_pos[0] = *reinterpret_cast<uint8_t *>((uint32_t)frame.pVirAddr[0] + i);
                    cur_pos[1] = *reinterpret_cast<uint8_t *>((uint32_t)frame.pVirAddr[0] + i + 1);
                    cur_pos[2] = *reinterpret_cast<uint8_t *>((uint32_t)frame.pVirAddr[1] + i);
                    cur_pos[3] = *reinterpret_cast<uint8_t *>((uint32_t)frame.pVirAddr[1] + i + 1);
                }

                AIFrame ai_frame;
                ai_frame.data = buf;
                ai_frame.ts = frame.u64TimeStamp;
                ai_frame.len = frame.u32Len * 2;
                {
                    std::unique_lock<std::mutex> lock;
                    for (size_t i = 0; i < sinks_.size(); i++)
                        sinks_[i]->OnFrame(ai_frame);
                }

                ret = HI_MPI_AI_ReleaseFrame(params_.dev, params_.chn, &frame, nullptr);
                if (ret != KSuccess)
                {
                    log_e("HI_MPI_AI_ReleaseFrame failed with %#x", ret);
                    return;
                }
            }
        }
    }));

    init_ = true;
    return KSuccess;
}

void AudioInput::Close()
{
    if (!init_)
        return;

    int ret;

    run_ = false;
    thread_->join();
    thread_.reset();
    thread_ = nullptr;

    ret = HI_MPI_AI_DisableChn(params_.dev, params_.chn);
    if (ret != KSuccess)
        log_e("HI_MPI_AI_DisableChn failed with %#x", ret);

    ret = HI_MPI_AI_Disable(params_.dev);
    if (ret != KSuccess)
        log_e("HI_MPI_AI_Disable failed with %#x", ret);

    StopTlv320();

    sinks_.clear();

    init_ = false;
}

void AudioInput::AddAudioSink(AudioSink<AIFrame> *sink)
{
    std::unique_lock<std::mutex> lock(mux_);
    sinks_.push_back(sink);
}

void AudioInput::RemoveAllAudioSink()
{
    std::unique_lock<std::mutex> lock(mux_);
    sinks_.clear();
}

} // namespace rs