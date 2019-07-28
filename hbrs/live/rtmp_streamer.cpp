#include "live/rtmp_streamer.h"
#include "common/err_code.h"

namespace rs
{

using namespace rtmp;

const int RTMPStreamer::DefaultTimeOut = 1000;

RTMPStreamer::RTMPStreamer() : init_(false)
{
}

RTMPStreamer::~RTMPStreamer()
{
    Close();
}

int RTMPStreamer::Initialize(const Params &params)
{
    if (init_)
        return KInitialized;

    int ret;

    params_ = params;

    rtmp_ = srs_rtmp_create(params_.url.c_str());
    if (rtmp_ == nullptr)
    {
        log_e("srs_rtmp_create failed");
        return KSDKError;
    }

    ret = srs_rtmp_set_timeout(rtmp_, DefaultTimeOut, DefaultTimeOut);
    if (ret != KSuccess)
    {
        log_e("srs_rtmp_set_timeout failed with %#x", ret);
        return KSDKError;
    }

    ret = srs_rtmp_handshake(rtmp_);
    if (ret != KSuccess)
    {
        log_e("srs_rtmp_handshake failed with %#x", ret);
        return KSDKError;
    }

    ret = srs_rtmp_connect_app(rtmp_);
    if (ret != KSuccess)
    {
        log_e("srs_rtmp_connect_app failed with %#x", ret);
        return KSDKError;
    }

    ret = srs_rtmp_publish_stream(rtmp_);
    if (ret != KSuccess)
    {
        log_e("srs_rtmp_publish_stream failed with %#x", ret);
        return KSDKError;
    }

    init_ = true;
    return KSuccess;
}

void RTMPStreamer::Close()
{
    if (!init_)
        return;

    init_ = false;
}

int RTMPStreamer::WriteAudioFrame(const AENCFrame &frame)
{
    if (!init_)
        return KUnInitialized;
    return KSuccess;
}

int RTMPStreamer::WriteVideoFrame(const VENCFrame &frame)
{
    if (!init_)
        return KUnInitialized;

    return KSuccess;
}

} // namespace rs