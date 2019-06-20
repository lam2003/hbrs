#pragma once

#include "common/global.h"
#include "system/vm.h"

namespace rs
{
namespace vo
{
struct Params
{
    int dev;
    int intf_type;
    int intf_sync;
};
} // namespace vo

class VideoOutput : public Module<vo::Params>
{
public:
    explicit VideoOutput();

    virtual ~VideoOutput(){};

    int Initialize(const vo::Params &params) override;

    void Close() override;

private:
};
} // namespace rs