#include "osd/osd.h"
#include "common/err_code.h"
#include "common/utils.h"

namespace rs
{

using namespace osd;

Osd::Osd() : font_(nullptr),
             init_(false)
{
}

Osd::~Osd()
{
    Close();
}

void Osd::Close()
{
    if (!init_)
        return;
    log_d("OSD stop,hdl:%d", params_.hdl);
    MPP_CHN_S mpp_chn;
    mpp_chn.enModId = HI_ID_GROUP;
    mpp_chn.s32DevId = params_.vpss_grp;
    mpp_chn.s32ChnId = 0;
    HI_MPI_RGN_DetachFrmChn(params_.hdl, &mpp_chn);
    HI_MPI_RGN_Destroy(params_.hdl);
    TTF_CloseFont(font_);
    init_ = false;
}

int Osd::Initialize(const Params &params)
{
    if (init_)
        return KInitialized;

    int ret;

    params_ = params;

    log_d("OSD start,hdl:%d", params_.hdl);

    TTF_Init();
    font_ = TTF_OpenFont(params_.font_file.c_str(), params_.font_size);
    if (!font_)
    {
        log_e("TTF_OpenFont open %s failed", params_.font_file.c_str());
        return KSDKError;
    }

    int font_w, font_h;

    TTF_SizeUTF8(font_, params_.content.c_str(), &font_w, &font_h);

    font_w = Utils::Align(font_w, 16);
    font_h = Utils::Align(font_h, 16);

    int x = params_.x;
    int y = params_.y;
    x = Utils::Align(x, 16);
    y = Utils::Align(y, 16);

    RGN_ATTR_S rgn_attr;
    memset(&rgn_attr, 0, sizeof(rgn_attr));
    rgn_attr.enType = OVERLAY_RGN;
    rgn_attr.unAttr.stOverlay.stSize.u32Width = font_w;
    rgn_attr.unAttr.stOverlay.stSize.u32Height = font_h;
    rgn_attr.unAttr.stOverlay.u32BgColor = 0x00000000;
    rgn_attr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;

    ret = HI_MPI_RGN_Create(params_.hdl, &rgn_attr);
    if (ret != KSuccess)
    {
        log_e("HI_MPI_RGN_Create failed with %#x", ret);
        return KSDKError;
    }

    RGN_CHN_ATTR_S chn_attr;
    memset(&chn_attr, 0, sizeof(chn_attr));
    chn_attr.bShow = HI_TRUE;
    chn_attr.enType = OVERLAY_RGN;
    chn_attr.unChnAttr.stOverlayChn.stPoint.s32X = x;
    chn_attr.unChnAttr.stOverlayChn.stPoint.s32Y = y;
    chn_attr.unChnAttr.stOverlayChn.u32BgAlpha = 0;
    chn_attr.unChnAttr.stOverlayChn.u32FgAlpha = 128;
    chn_attr.unChnAttr.stOverlayChn.u32Layer = 0;
    chn_attr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
    chn_attr.unChnAttr.stOverlayChn.stQpInfo.s32Qp = 0;

    MPP_CHN_S mpp_chn;
    mpp_chn.enModId = HI_ID_GROUP;
    mpp_chn.s32DevId = params_.vpss_grp;
    mpp_chn.s32ChnId = 0;
    ret = HI_MPI_RGN_AttachToChn(params_.hdl, &mpp_chn, &chn_attr);
    if (ret != HI_SUCCESS)
    {
        log_e("HI_MPI_RGN_AttachToChn failed with %#x", ret);
        return KSDKError;
    }

    SDL_Color forecol = {params_.font_color_r, params_.font_color_g, params_.font_color_b, 0};

    SDL_Surface *sdl_txtsurface = TTF_RenderUTF8_Solid(font_, params_.content.c_str(), forecol);
    if (sdl_txtsurface == nullptr)
    {
        log_e("TTF_RenderUTF8_Solid failed");
        return KSDKError;
    }

    SDL_Surface *sdl_tmpsurface = SDL_CreateRGBSurface(SDL_SWSURFACE, sdl_txtsurface->w, sdl_txtsurface->h, 16, 0x00007c00, 0x000003e0, 0x0000001f, 0x00008000);
    if (sdl_tmpsurface == nullptr)
    {
        log_d("SDL_CreateRGBSurface failed");
        return KSDKError;
    }

    SDL_Rect bounds;
    bounds.x = 0;
    bounds.y = 0;
    bounds.w = sdl_txtsurface->w;
    bounds.h = sdl_txtsurface->h;
    if (SDL_LowerBlit(sdl_txtsurface, &bounds, sdl_tmpsurface, &bounds) < 0)
    {
        log_e("SDL_LowerBlit failed");
        return KSDKError;
    }

    BITMAP_S bitmap;
    bitmap.u32Width = sdl_tmpsurface->pitch / 2;
    bitmap.u32Height = sdl_tmpsurface->h;
    bitmap.pData = sdl_tmpsurface->pixels;
    bitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;

    ret = HI_MPI_RGN_SetBitMap(params_.hdl, &bitmap);
    if (ret != HI_SUCCESS)
    {
        log_e("HI_MPI_RGN_SetBitMap failed with %#x", ret);
        return KSDKError;
    }

    if (bitmap.pData != nullptr)
        free(bitmap.pData);
    SDL_FreeSurface(sdl_txtsurface);

    init_ = true;
    return KSuccess;
}

} // namespace rs