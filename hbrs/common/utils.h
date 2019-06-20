#pragma once

#include <chrono>
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

    static uint64_t GetSteadyMilliSeconds()
    {
        using namespace std::chrono;
        auto now = steady_clock::now();
        auto now_since_epoch = now.time_since_epoch();
        return duration_cast<milliseconds>(now_since_epoch).count();
    }
};
} // namespace rs