#include "common/global.h"

#pragma once

namespace rs
{

namespace vdec
{
struct Params
{
    int32_t chn;
    int width;
    int height;
};

} // namespace vdec

class VideoDecode : public VideoSink<VDEC_STREAM_S>
{
public:
    explicit VideoDecode();

    virtual ~VideoDecode();

    int32_t Initialize(const vdec::Params &params);

    void Close();

    void OnFrame(const VDEC_STREAM_S &st, int chn) override;

private:
    vdec::Params params_;
    bool init_;
};
} // namespace rs