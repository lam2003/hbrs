#pragma once

#include "common/global.h"

namespace rs
{
class MPPSystem
{
public:
    struct Params
    {
        CaptureMode mode;
        int block_num;
    };

    static MPPSystem *Instance();

    virtual ~MPPSystem();

    int Initialize(const Params &params);

    void Close();

protected:
    static int ConfigVB(CaptureMode mode, int block_num);

    static int ConfigSys(int align_width);

    static void ConfigLogger();

    static int ConfigMem();

private:
    explicit MPPSystem();

private:
    Params params_;
    bool init_;
};
} // namespace rs