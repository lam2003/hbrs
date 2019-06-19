#pragma once

#include "common/global.h"

namespace rs
{
class VideoProcess
{
public:
    struct Params
    {
        CaptureMode mode;
        int grp;
    };

    explicit VideoProcess();

    virtual ~VideoProcess();

    int Initialize(const Params &params);

    void Close();

    int SetEncodeChnSize(const SIZE_S &size);

    int SetPIPChnSize(const SIZE_S &size);

protected:
    static int SetChnMode(int grp, int chn, const SIZE_S &size);

private:
    Params params_;
    bool init_;

    static const int VpssEncodeChn;
    static const int VpssPIPChn;
};
} // namespace rs