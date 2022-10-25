#pragma once

#include "global.h"

namespace rs
{
namespace fb
{
struct Params
{
    std::string dev;
    VO_INTF_SYNC_E intf_sync;
};
} // namespace fb

class FrameBuffer
{
public:
    explicit FrameBuffer();

    virtual ~FrameBuffer();

    int Initialize(const fb::Params &params);

    void Close();

private:
    fb::Params params_;
    int fd_;
    bool init_;
};

} // namespace rs