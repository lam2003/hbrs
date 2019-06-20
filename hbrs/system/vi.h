#include "common/global.h"

#pragma once

namespace rs
{

namespace vi
{
struct Params
{
    int dev;
    int chn;
    CaptureMode mode;
};

} // namespace vi

class VideoInput
{
public:
    explicit VideoInput();

    virtual ~VideoInput();

    int Initialize(const vi::Params &params);

    void Close();

    int GetDev() const;

    int GetChn() const;

protected:
    static int StartDev(int dev, CaptureMode mode);

    static void SetMask(int dev, VI_DEV_ATTR_S &dev_attr);

    static int StartChn(int chn, CaptureMode mode);

private:
    vi::Params params_;
    bool init_;

    static const VI_DEV_ATTR_S DevAttr_7441_BT1120_720P;
    static const VI_DEV_ATTR_S DevAttr_7441_BT1120_1080P;
};
} // namespace rs