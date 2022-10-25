#include "live/normal_rtmp_streamer.h"
#include "common/err_code.h"

#define RTMP_HEAD_SIZE (sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE)

namespace rs
{
NormalRtmpStreamer::NormalRtmpStreamer() : buffer_(2 * 1024 * 1024),
                                 rtmp_(nullptr),
                                 sps_(""),
                                 pps_(""),
                                 send_sps_pps_(false),
                                 has_audio_(false),
                                 init_(false)
{
}

NormalRtmpStreamer::~NormalRtmpStreamer()
{
    Close();
}

bool NormalRtmpStreamer::SendSpsPps(const std::string &sps, const std::string &pps)
{
    RTMPPacket *packet;
    uint8_t *body;

    packet = (RTMPPacket *)buffer_.vir_addr;
    packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
    body = (uint8_t *)packet->m_body;

    int i = 0;
    body[i++] = 0x17;
    body[i++] = 0x00;

    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;

    body[i++] = 0x01;
    body[i++] = sps[5];
    body[i++] = sps[6];
    body[i++] = sps[7];
    body[i++] = 0xff;

    body[i++] = 0xe1;
    body[i++] = ((sps.length() - 4) >> 8) & 0xff;
    body[i++] = (sps.length() - 4) & 0xff;
    memcpy(&body[i], sps.c_str() + 4, sps.length() - 4);
    i += (sps.length() - 4);

    body[i++] = 0x01;
    body[i++] = ((pps.length() - 4) >> 8) & 0xff;
    body[i++] = (pps.length() - 4) & 0xff;
    memcpy(&body[i], pps.c_str() + 4, pps.length() - 4);
    i += (pps.length() - 4);

    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = i;
    packet->m_nChannel = 0x04;
    packet->m_nTimeStamp = 0;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    packet->m_nInfoField2 = rtmp_->m_stream_id;

    return RTMP_SendPacket(rtmp_, packet, true);
}

bool NormalRtmpStreamer::SendVideoData(const VENCFrame &frame)
{
    RTMPPacket *packet;
    uint8_t *body;
    bool keyframe;

    keyframe = false;
    if (frame.type == H264E_NALU_ISLICE)
        keyframe = true;

    packet = (RTMPPacket *)buffer_.vir_addr;
    packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
    body = (uint8_t *)packet->m_body;

    int i = 0;
    body[i++] = keyframe ? 0x17 : 0x27;
    body[i++] = 0x01;
    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;

    body[i++] = (frame.len - 4) >> 24 & 0xff;
    body[i++] = (frame.len - 4) >> 16 & 0xff;
    body[i++] = (frame.len - 4) >> 8 & 0xff;
    body[i++] = (frame.len - 4) & 0xff;

    memcpy(&body[i], frame.data + 4, frame.len - 4);

    i += (frame.len - 4);

    packet->m_nBodySize = i;
    packet->m_hasAbsTimestamp = 0;
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nInfoField2 = rtmp_->m_stream_id;
    packet->m_nChannel = 0x04;
    packet->m_nTimeStamp = frame.ts;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;

    return RTMP_SendPacket(rtmp_, packet, true);
}

uint16_t GetAACMeta()
{
    uint16_t res;
    uint16_t *p;

    p = &res;

    *p = 0;
    *p |= (2 << 11);
    *p |= (3 << 7);
    *p |= (2 << 3);

    uint8_t temp = *p >> 8;
    *p <<= 8;
    *p |= temp;

    return res;
}

bool NormalRtmpStreamer::SendAACMeta()
{
    RTMPPacket *packet;
    uint8_t *body;

    packet = (RTMPPacket *)buffer_.vir_addr;
    packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
    body = (uint8_t *)packet->m_body;

    body[0] = 0xAF;
    body[1] = 0x00;

    uint16_t aac_meta = GetAACMeta();
    memcpy(&body[2], &aac_meta, sizeof(uint16_t));

    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nBodySize = 4;
    packet->m_nChannel = 0x04;
    packet->m_nTimeStamp = 0;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_nInfoField2 = rtmp_->m_stream_id;

    return RTMP_SendPacket(rtmp_, packet, true);
}

int NormalRtmpStreamer::Initialize(const std::string &url, bool has_audio)
{

    if (init_)
        return KInitialized;

    url_ = url;
    has_audio_ = has_audio;

    static std::mutex g_Mux;
    g_Mux.lock();
    rtmp_ = RTMP_Alloc();
    RTMP_Init(rtmp_);
    g_Mux.unlock();

    if (!RTMP_SetupURL(rtmp_, const_cast<char *>(url.c_str())))
    {
        RTMP_Free(rtmp_);
        log_e("[%s]RTMP_SetupURL failed", url.c_str());
        return KSDKError;
    }

    RTMP_EnableWrite(rtmp_);

    if (!RTMP_Connect(rtmp_, NULL))
    {
        RTMP_Close(rtmp_);
        RTMP_Free(rtmp_);
        log_e("[%s]RTMP_EnableWrite failed", url.c_str());
        return KSDKError;
    }

    if (!RTMP_ConnectStream(rtmp_, 0))
    {
        RTMP_Close(rtmp_);
        RTMP_Free(rtmp_);
        log_e("[%s]RTMP_ConnectStream failed", url.c_str());
        return KSDKError;
    }

    init_ = true;

    return KSuccess;
}

int NormalRtmpStreamer::WriteVideoFrame(const VENCFrame &frame)
{
    if (!init_)
        return KUnInitialized;

    if (!send_sps_pps_ && frame.type == H264E_NALU_SPS)
    {
        sps_ = std::string((char *)frame.data, frame.len);
    }
    else if (!send_sps_pps_ && frame.type == H264E_NALU_PPS)
    {
        pps_ = std::string((char *)frame.data, frame.len);
    }

    if (!send_sps_pps_ && sps_ != "" && pps_ != "")
    {
        if (has_audio_)
        {
            if (!SendAACMeta())
            {
                log_e("[%s]SendAACMeta failed", url_.c_str());
                return KSDKError;
            }
        }
        if (!SendSpsPps(sps_, pps_))
        {
            log_e("[%s]SendSpsPps failed", url_.c_str());
            return KSDKError;
        }
        send_sps_pps_ = true;
    }

    if (frame.type == H264E_NALU_SPS || frame.type == H264E_NALU_PPS || frame.type == H264E_NALU_SEI)
        return KSuccess;

    if (!SendVideoData(frame))
    {
        log_e("[%s]SendVideoData failed", url_.c_str());
        return KSDKError;
    }

    return KSuccess;
}

bool NormalRtmpStreamer::SendAudioData(const AENCFrame &frame)
{
    RTMPPacket *packet;
    uint8_t *body;

    packet = (RTMPPacket *)buffer_.vir_addr;
    packet->m_body = (char *)(packet + RTMP_HEAD_SIZE);
    body = (uint8_t *)packet->m_body;

    int i = 0;
    body[i++] = 0xAF;
    body[i++] = 0x01;

    memcpy(&body[i], frame.data + 7, frame.len - 7);
    i += (frame.len - 7);

    packet->m_nBodySize = i;
    packet->m_hasAbsTimestamp = 0;
    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nInfoField2 = rtmp_->m_stream_id;
    packet->m_nChannel = 0x04;
    packet->m_nTimeStamp = frame.ts;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;

    return RTMP_SendPacket(rtmp_, packet, true);
}

int NormalRtmpStreamer::WriteAudioFrame(const AENCFrame &frame)
{
    if (!init_)
        return KUnInitialized;

    if (!send_sps_pps_)
        return KSuccess;

    if (!SendAudioData(frame))
    {
        log_e("[%s]SendAudioData failed", url_.c_str());
        return KSDKError;
    }

    return KSuccess;
}

void NormalRtmpStreamer::Close()
{
    if (!init_)
        return;

    RTMP_Close(rtmp_);
    RTMP_Free(rtmp_);
    rtmp_ = nullptr;
    init_ = false;
}

} // namespace rs