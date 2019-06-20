#pragma once

#include "common/global.h"
#include "system/vi.h"
#include "system/vpss.h"

namespace rs
{

namespace mpp
{
struct Params
{
    CaptureMode mode;
    int block_num;
};
} // namespace mpp

class MPPSystem : public Module<mpp::Params>
{
public:
    static MPPSystem *Instance();

    virtual ~MPPSystem();

    int Initialize(const mpp::Params &params);

    void Close();

    static int Bind(const VideoInput &vi, const VideoProcess &vpss);

    static int UnBind(const VideoInput &vi, const VideoProcess &vpss);

protected:
    static int ConfigVB(CaptureMode mode, int block_num);

    static int ConfigSys(int align_width);

    static void ConfigLogger();

    static int ConfigMem();

private:
    explicit MPPSystem();

private:
    mpp::Params params_;
    bool init_;
};
} // namespace rs