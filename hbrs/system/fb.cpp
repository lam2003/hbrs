#include "system/fb.h"
#include "common/err_code.h"
#include "common/utils.h"

static fb_bitfield g_R32 = {16, 8, 0};
static fb_bitfield g_G32 = {8, 8, 0};
static fb_bitfield g_B32 = {0, 8, 0};
static fb_bitfield g_A32 = {24, 8, 0};

namespace rs
{

using namespace fb;

FrameBuffer::FrameBuffer() : fd_(0),
                             init_(false)
{
}

FrameBuffer::~FrameBuffer()
{
    Close();
}

void FrameBuffer::Close()
{
    if (!init_)
        return;

    close(fd_);
    init_ = false;
}

int FrameBuffer::Initialize(const fb::Params &params)
{
    if (init_)
        return KInitialized;

    params_ = params;

    fd_ = open(params_.dev.c_str(), O_RDWR, 0);
    if (fd_ < 0)
    {
        log_e("open %s failed,%s", params_.dev.c_str(), strerror(errno));
        return KSystemError;
    }

    HI_BOOL show = HI_FALSE;
    if (ioctl(fd_, FBIOPUT_SHOW_HIFB, &show) < 0)
    {
        log_e("ioctl FBIOPUT_SHOW_HIFB failed,%s", strerror(errno));
        return KSystemError;
    }

    HIFB_POINT_S point = {0, 0};
    if (ioctl(fd_, FBIOPUT_SCREEN_ORIGIN_HIFB, &point) < 0)
    {
        log_e("ioctl FBIOPUT_SCREEN_ORIGIN_HIFB failed,%s", strerror(errno));
        return KSystemError;
    }

    HIFB_ALPHA_S alpha;
    memset(&alpha, 0, sizeof(alpha));
    alpha.bAlphaEnable = HI_TRUE;
    alpha.bAlphaChannel = HI_FALSE;
    alpha.u8Alpha0 = 0x0;
    alpha.u8Alpha1 = 0xff;
    alpha.u8GlobalAlpha = 0xff;

    if (ioctl(fd_, FBIOPUT_ALPHA_HIFB, &alpha) < 0)
    {
        log_e("ioctl FBIOPUT_ALPHA_HIFB failed,%s", strerror(errno));
        return KSystemError;
    }

    fb_var_screeninfo info;
    memset(&info, 0, sizeof(info));
    if (ioctl(fd_, FBIOGET_VSCREENINFO, &info) < 0)
    {
        log_e("ioctl FBIOGET_VSCREENINFO failed,%s", strerror(errno));
        return KSystemError;
    }

    SIZE_S size = Utils::GetSize(params_.intf_sync);
    info.xres = size.u32Width;
    info.yres = size.u32Height;
    info.xres_virtual = size.u32Width;
    info.yres_virtual = size.u32Height;
    info.transp = g_A32;
    info.red = g_R32;
    info.green = g_G32;
    info.blue = g_B32;
    info.bits_per_pixel = 32;
    info.activate = FB_ACTIVATE_NOW;

    if (ioctl(fd_, FBIOPUT_VSCREENINFO, &info) < 0)
    {
        log_e("ioctl FBIOPUT_VSCREENINFO failed,%s", strerror(errno));
        return KSystemError;
    }

    show = HI_TRUE;
    if (ioctl(fd_, FBIOPUT_SHOW_HIFB, &show) < 0)
    {
        log_e("ioctl FBIOPUT_SHOW_HIFB failed,%s", strerror(errno));
        return KSystemError;
    }

    init_ = true;

    return KSuccess;
}

} // namespace rs