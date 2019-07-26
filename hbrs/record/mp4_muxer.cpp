#include "record/mp4_muxer.h"
#include "common/err_code.h"

namespace rs
{

MP4Muxer::MP4Muxer() : sps_(""),
                       pps_(""),
                       sei_(""),
                       vts_base(0),
                       ats_base(0),
                       ctx_(0),
                       init_(false)
{
}

MP4Muxer::~MP4Muxer()
{
    Close();
}

// int get_sr_index(unsigned int sampling_frequency)
// {
//     switch (sampling_frequency)
//     {
//     case 96000:
//         return 0;
//     case 88200:
//         return 1;
//     case 64000:
//         return 2;
//     case 48000:
//         return 3;
//     case 44100:
//         return 4;
//     case 32000:
//         return 5;
//     case 24000:
//         return 6;
//     case 22050:
//         return 7;
//     case 16000:
//         return 8;
//     case 12000:
//         return 9;
//     case 11025:
//         return 10;
//     case 8000:
//         return 11;
//     case 7350:
//         return 12;
//     default:
//         return 0;
//     }
// }

// void make_dsi(unsigned int sampling_frequency_index, unsigned int channel_configuration, unsigned char *dsi)
// {
//     unsigned int object_type = 2; // AAC LC by default
//     dsi[0] = (object_type << 3) | (sampling_frequency_index >> 1);
//     dsi[1] = ((sampling_frequency_index & 1) << 7) | (channel_configuration << 3);
// }

int MP4Muxer::Initialize(int width, int height, int frame_rate, int samplate_rate, const std::string filename)
{
    if (init_)
        return KInitialized;

    int ret;

    sps_ = "";
    pps_ = "";
    sei_ = "";
    vts_base = 0;
    ats_base = 0;

    allocator_2048k::mmz_malloc(mmz_bufer_);

    ctx_ = avformat_alloc_context();
    if (!ctx_)
    {
        log_e("avformat_alloc_context failed");
        return KSDKError;
    }

    strcpy(ctx_->filename, filename.c_str());
    ctx_->oformat = av_guess_format(nullptr, ".mp4", nullptr);
    if (!ctx_->oformat)
    {
        log_e("av_guess_format failed");
        return KSDKError;
    }

    av_register_output_format(ctx_->oformat);

    AVStream *video_stream = avformat_new_stream(ctx_, nullptr);
    if (!video_stream)
    {
        log_e("avformat_new_stream failed");
        return KSDKError;
    }
    video_stream->id = 0;
    video_stream->index = 0;
    video_stream->time_base = {1, 1000000};
    video_stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    video_stream->codecpar->codec_id = AV_CODEC_ID_H264;
    video_stream->codecpar->width = width;
    video_stream->codecpar->height = height;
    ctx_->streams[0] = video_stream;

    AVStream *audio_stream = avformat_new_stream(ctx_, nullptr);
    if (audio_stream == nullptr)
    {
        log_e("avformat_new_stream failed");
        return KSDKError;
    }
    audio_stream->id = 1;
    audio_stream->index = 1;
    audio_stream->time_base = (AVRational){1, 1000000};
    audio_stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    audio_stream->codecpar->codec_id = AV_CODEC_ID_AAC;
    audio_stream->codecpar->channels = 2;
    audio_stream->codecpar->channel_layout = AV_CH_LAYOUT_STEREO;
    audio_stream->codecpar->frame_size = 4096;
    audio_stream->codecpar->format = AV_SAMPLE_FMT_S16;
    audio_stream->codecpar->sample_rate = samplate_rate;
    audio_stream->codecpar->profile = FF_PROFILE_AAC_LOW;
    ctx_->streams[1] = audio_stream;

    ret = avio_open(&ctx_->pb, filename.c_str(), AVIO_FLAG_WRITE);
    if (ret != 0)
    {
        log_e("avio_open failed");
        return KSDKError;
    }

    ret = avformat_write_header(ctx_, nullptr);
    if (ret != 0)
    {
        log_e("avformat_write_header failed");
        return KSDKError;
    }

    init_ = true;
    return KSuccess;
}

void MP4Muxer::Close()
{
    if (!init_)
        return;
    av_write_trailer(ctx_);
    avcodec_parameters_free(&ctx_->streams[0]->codecpar);
    avcodec_parameters_free(&ctx_->streams[1]->codecpar);
    avio_close(ctx_->pb);
    avformat_free_context(ctx_);
    allocator_2048k::mmz_free(mmz_bufer_);
    init_ = false;
}

int MP4Muxer::WriteVideoFrame(const VENCFrame &frame)
{
    if (!init_)
        return KUnInitialized;

    int32_t ret;
    if (sps_ == "" || pps_ == "" || sei_ == "")
    {
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
        return static_cast<int>(KSuccess);
    }

    if (frame.type == H264E_NALU_SPS || frame.type == H264E_NALU_PPS || frame.type == H264E_NALU_SEI)
        return static_cast<int>(KSuccess);

    AVPacket pkt;
    av_init_packet(&pkt);

    pkt.data = mmz_bufer_.vir_addr;

    if (frame.type == H264E_NALU_ISLICE)
    {
        uint32_t pos = 0;
        memcpy(pkt.data, sps_.c_str(), sps_.length());
        pos += sps_.length();
        memcpy(pkt.data + pos, pps_.c_str(), pps_.length());
        pos += pps_.length();
        memcpy(pkt.data + pos, sei_.c_str(), sei_.length());
        pos += sei_.length();
        memcpy(pkt.data + pos, frame.data, frame.len);
        pos += frame.len;
        pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.size = pos;
    }
    else
    {
        memcpy(pkt.data, frame.data, frame.len);
        pkt.size = frame.len;
    }

    if (vts_base == 0)
        vts_base = frame.ts;
    pkt.pts = frame.ts - vts_base;
    pkt.dts = pkt.pts;
    pkt.stream_index = 0;

    av_packet_rescale_ts(&pkt, {1, 1000000}, ctx_->streams[0]->time_base);
    ret = av_interleaved_write_frame(ctx_, &pkt);
    av_packet_unref(&pkt);
    if (ret != 0)
    {

        log_e("av_interleaved_write_frame failed");
        return KSDKError;
    }

    return KSuccess;
}

int MP4Muxer::WriteAudioFrame(const AENCFrame &frame)
{
    if (!init_)
        return KUnInitialized;

    int ret;
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.size = frame.len;
    pkt.data = frame.data;
    if (ats_base == 0)
        ats_base = frame.ts;
    pkt.pts = frame.ts - ats_base;
    pkt.dts = pkt.pts;
    pkt.stream_index = 1;

    av_packet_rescale_ts(&pkt, {1, 1000000}, ctx_->streams[1]->time_base);
    ret = av_interleaved_write_frame(ctx_, &pkt);
    av_packet_unref(&pkt);
    if (ret != 0)
    {
        log_e("av_interleaved_write_frame failed");
        return KSDKError;
    }

    return KSuccess;
}

} // namespace rs