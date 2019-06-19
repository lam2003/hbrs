#pragma once

#include "common/global.h"

namespace rs
{
class Utils
{
public:
    static SIZE_S GetSize(CaptureMode mode)
    {
        SIZE_S size;

        switch (mode)
        {
        case CAPTURE_MODE_720P:
            size.u32Width = 1280;
            size.u32Height = 720;
            break;
        case CAPTURE_MODE_1080P:
            size.u32Width = 1920;
            size.u32Height = 1080;
            break;
        default:
            RS_ASSERT(0);
        }

        return size;
    }
};
} // namespace rs