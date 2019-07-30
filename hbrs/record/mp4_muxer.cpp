#include "record/mp4_muxer.h"
#include "common/err_code.h"
#include "common/utils.h"

namespace rs
{

MP4Muxer::MP4Muxer() : hdl_(MP4_INVALID_FILE_HANDLE),
                       vtrack_(MP4_INVALID_TRACK_ID),
                       atrack_(MP4_INVALID_TRACK_ID),
                       width_(0),
                       height_(0),
                       frame_rate_(0),
                       samplate_rate_(0),
                       sps_(""),
                       pps_(""),
                       sei_(""),
                       vts_base_(0),
                       set_ts_(false),
                       aone_frame_duration_(0),
                       vone_frame_duration_(0),
                       aduration_(0),
                       vduration_(0),
                       init_ctx_(false),
                       init_(false)

{
}

MP4Muxer::~MP4Muxer()
{
    Close();
}

int MP4Muxer::Initialize(int width, int height, int frame_rate, int samplate_rate, const std::string filename, bool set_ts)
{
    if (init_)
        return KInitialized;

    allocator_2048k::mmz_malloc(mmz_buffer_);
    hdl_ = MP4Create(filename.c_str(), MP4_CREATE_64BIT_TIME | MP4_CREATE_64BIT_DATA | MP4_CLOSE_DO_NOT_COMPUTE_BITRATE);
    if (hdl_ == MP4_INVALID_FILE_HANDLE)
    {
        log_e("MP4Create failed");
        return KSDKError;
    }

    if (!MP4SetTimeScale(hdl_, 1000000))
    {
        log_e("MP4SetTimeScale failed");
        return KSDKError;
    }

    width_ = width;
    height_ = height;
    frame_rate_ = frame_rate;
    samplate_rate_ = samplate_rate;

    sps_ = "";
    pps_ = "";
    sei_ = "";
    vts_base_ = 0;

    set_ts_ = set_ts;
    aone_frame_duration_ = (1024 * 1000000) / samplate_rate_;
    vone_frame_duration_ = 1000000 / frame_rate_;
    aduration_ = 0;
    vduration_ = 0;

    init_ctx_ = false;

    init_ = true;

    return KSuccess;
}

int MP4Muxer::InitContext()
{
    if (!init_)
        return KUnInitialized;

    atrack_ = MP4AddAudioTrack(hdl_, samplate_rate_, 1024, MP4_MPEG4_AUDIO_TYPE);
    if (atrack_ == MP4_INVALID_TRACK_ID)
    {
        log_e("MP4AddAudioTrack failed");
        return KSDKError;
    }

    MP4SetAudioProfileLevel(hdl_, 0x2);

    uint8_t spec[2];
    Utils::GetAACSpec(samplate_rate_, 2, spec);
    if (!MP4SetTrackESConfiguration(hdl_, atrack_, spec, 2))
    {
        log_e("MP4SetTrackESConfiguration failed");
        return KSDKError;
    }

    vtrack_ = MP4AddH264VideoTrack(hdl_, 1000000, 1000000 / frame_rate_, width_, height_,
                                   sps_[5],
                                   sps_[6],
                                   sps_[7],
                                   3);
    if (vtrack_ == MP4_INVALID_TRACK_ID)
    {
        log_e("MP4AddH264VideoTrack failed");
        return KSDKError;
    }

    MP4SetVideoProfileLevel(hdl_, 0x7F);
    MP4AddH264SequenceParameterSet(hdl_, vtrack_, reinterpret_cast<const uint8_t *>(sps_.c_str() + 4), sps_.length() - 4);
    MP4AddH264PictureParameterSet(hdl_, vtrack_, reinterpret_cast<const uint8_t *>(pps_.c_str() + 4), pps_.length() - 4);
    MP4AddH264PictureParameterSet(hdl_, vtrack_, reinterpret_cast<const uint8_t *>(sei_.c_str() + 4), sei_.length() - 4);

    init_ctx_ = true;

    return KSuccess;
}

void MP4Muxer::Close()
{
    if (!init_)
        return;

    MP4Close(hdl_);
    allocator_2048k::mmz_free(mmz_buffer_);
    init_ = false;
}

int MP4Muxer::WriteVideoFrame(const VENCFrame &frame)
{
    if (!init_)
        return KUnInitialized;

    if (frame.type == H264E_NALU_SPS)
    {
        sps_ = std::string((char *)frame.data, frame.len);
    }
    else if (frame.type == H264E_NALU_PPS)
    {
        pps_ = std::string((char *)frame.data, frame.len);
    }
    else if (frame.type == H264E_NALU_SEI)
    {
        sei_ = std::string((char *)frame.data, frame.len);
    }

    if (!init_ctx_ && sps_ != "" && pps_ != "" && sei_ != "")
        return InitContext();

    if (frame.type == H264E_NALU_SPS || frame.type == H264E_NALU_PPS || frame.type == H264E_NALU_SEI)
        return KSuccess;

    uint64_t duration;
    if (set_ts_)
    {
        duration = vone_frame_duration_;
        vduration_ += duration;
    }
    else
    {
        if (vts_base_ == 0)
            vts_base_ = frame.ts;
        duration = frame.ts - vts_base_;
        vts_base_ = frame.ts;
    }

    uint32_t *tmp = reinterpret_cast<uint32_t *>(mmz_buffer_.vir_addr);
    uint8_t *buf = mmz_buffer_.vir_addr + 4;
    uint32_t pos = 0;
    if (frame.type == H264E_NALU_ISLICE)
    {
        memcpy(buf, sps_.c_str(), sps_.length());
        pos += sps_.length();
        memcpy(buf + pos, pps_.c_str(), pps_.length());
        pos += pps_.length();
        memcpy(buf + pos, sei_.c_str(), sei_.length());
        pos += sei_.length();
        memcpy(buf + pos, frame.data, frame.len);
        pos += frame.len;
        *tmp = htonl(pos);

        if (set_ts_ && aduration_ > vduration_)
        {
            duration += (aduration_ - vduration_);
            vduration_ = aduration_;

        }

        if (!MP4WriteSample(hdl_, vtrack_, mmz_buffer_.vir_addr, pos + 4, duration, 0, true))
        {
            log_e("MP4WriteSample failed");
            return KSDKError;
        }
    }
    else if (frame.type == H264E_NALU_PSLICE)
    {
        memcpy(buf, frame.data, frame.len);
        pos += frame.len;
        *tmp = htonl(pos);
        if (!MP4WriteSample(hdl_, vtrack_, mmz_buffer_.vir_addr, pos + 4, duration, 0, false))
        {
            log_e("MP4WriteSample failed");
            return KSDKError;
        }
    }

    return KSuccess;
}

int MP4Muxer::WriteAudioFrame(const AENCFrame &frame)
{
    if (!init_)
        return KUnInitialized;

    if (!init_ctx_)
        return KSuccess;

    if (set_ts_)
        aduration_ += aone_frame_duration_;

    if (!MP4WriteSample(hdl_, atrack_, frame.data, frame.len, MP4_INVALID_DURATION))
    {
        log_e("MP4WriteSample failed");
        return KSDKError;
    }

    return KSuccess;
}

} // namespace rs