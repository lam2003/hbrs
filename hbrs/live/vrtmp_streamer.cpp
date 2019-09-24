#include "live/vrtmp_streamer.h"
#include "common/err_code.h"

#define RTMP_HEAD_SIZE (sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE)

namespace rs
{
VRtmpStreamer::VRtmpStreamer() : buffer_(2 * 1024 * 1024),
                                 rtmp_(nullptr),
                                 sps_(""),
                                 pps_(""),
                                 send_sps_pps_(false),
                                 init_(false)
{
}

VRtmpStreamer::~VRtmpStreamer()
{
    Close();
}

bool VRtmpStreamer::SendSpsPps(const std::string &sps, const std::string &pps)
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

bool VRtmpStreamer::SendVideoData(const VENCFrame &frame)
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

int VRtmpStreamer::Initialize(const std::string &url)
{

    if (init_)
        return KInitialized;

    url_ = url;

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

int VRtmpStreamer::WriteVideoFrame(const VENCFrame &frame)
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

void VRtmpStreamer::Close()
{
    if (!init_)
        return;

    RTMP_Close(rtmp_);
    RTMP_Free(rtmp_);
    rtmp_ = nullptr;
    init_ = false;
}

} // namespace rs