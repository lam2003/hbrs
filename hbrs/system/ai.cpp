#include "system/ai.h"
#include "common/err_code.h"
#include "common/buffer.h"

namespace rs
{

using namespace ai;

const int AudioInput::BufferLen = 4 * 1024;

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
    log_d("AI start,dev:%d,chn:%d", params.dev, params.chn);
    params_ = params;

    AIO_ATTR_S attr;
    memset(&attr, 0, sizeof(attr));
    attr.enSamplerate = AUDIO_SAMPLE_RATE_48000;
    attr.enBitwidth = AUDIO_BIT_WIDTH_16;
    attr.enWorkmode = AIO_MODE_I2S_SLAVE;
    attr.u32EXFlag = 1;
    attr.u32FrmNum = 30;
    attr.u32PtNumPerFrm = 320;
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

    param.u32UsrFrmDepth = 30;
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

        // uint8_t *buf = reinterpret_cast<uint8_t *>(malloc(BufferLen));
        // uint32_t buf_len = BufferLen;
        MMZBuffer buffer(BufferLen);

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

                // if (frame.u32Len * 2 > buf_len)
                // {
                //     free(buf);
                //     buf = reinterpret_cast<uint8_t *>(malloc(frame.u32Len * 2));
                //     buf_len = frame.u32Len * 2;
                // }

                // uint8_t *cur_pos = buf;
                uint8_t *cur_pos = buffer.vir_addr;
                for (uint32_t i = 0; i < frame.u32Len; i += 2)
                {
                    cur_pos[0] = *reinterpret_cast<uint8_t *>((uint32_t)frame.pVirAddr[0] + i);
                    cur_pos[1] = *reinterpret_cast<uint8_t *>((uint32_t)frame.pVirAddr[0] + i + 1);
                    cur_pos[2] = *reinterpret_cast<uint8_t *>((uint32_t)frame.pVirAddr[1] + i);
                    cur_pos[3] = *reinterpret_cast<uint8_t *>((uint32_t)frame.pVirAddr[1] + i + 1);
                    cur_pos += 4;
                }

                // AIFrame ai_frame;
                // ai_frame.data = buf;
                // ai_frame.ts = frame.u64TimeStamp;
                // ai_frame.len = frame.u32Len * 2;
                {
                    std::unique_lock<std::mutex> lock;
                    for (size_t i = 0; i < sinks_.size(); i++)
                        // sinks_[i]->OnFrame(ai_frame);
                        sinks_[i]->OnFrame(buffer.vir_addr, frame.u32Len * 2);
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
    log_d("AI stop,dev:%d,chn:%d", params_.dev, params_.chn);
    run_ = false;
    thread_->join();
    thread_.reset();
    thread_ = nullptr;
#if 1
    int ret;
    ret = HI_MPI_AI_DisableChn(params_.dev, params_.chn);
    if (ret != KSuccess)
        log_e("HI_MPI_AI_DisableChn failed with %#x", ret);

    ret = HI_MPI_AI_Disable(params_.dev);
    if (ret != KSuccess)
        log_e("HI_MPI_AI_Disable failed with %#x", ret);
#endif

    sinks_.clear();

    init_ = false;
}

void AudioInput::AddAudioSink(std::shared_ptr<AudioSink<AIFrame>> sink)
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