#include "common/global.h"

#pragma once

namespace rs
{
class VideoInput
{
public:
    struct Params
    {
        int dev;
        int chn;
        CaptureMode mode;
    };

    explicit VideoInput();

    virtual ~VideoInput();

    int Initialize(const Params &params);

    void Close();

protected:
    static int StartDev(int dev, CaptureMode mode);

    static void SetMask(int dev, VI_DEV_ATTR_S &dev_attr);

    static int StartChn(int chn, CaptureMode mode);

private:
    Params params_;
    bool init_;

    static const VI_DEV_ATTR_S DevAttr_7441_BT1120_720P;
    static const VI_DEV_ATTR_S DevAttr_7441_BT1120_1080P;
};
} // namespace rs