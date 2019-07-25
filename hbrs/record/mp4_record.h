#pragma once

#include "record/mp4_muxer.h"

namespace rs
{

class MP4Record : public AudioSink<AENCFrame>, public VideoSink<VENCFrame>
{
public:
    explicit MP4Record();

    virtual ~MP4Record();

    int Initialize(const mp4_muxer::Params &params);

    void Close();

    void OnFrame(const AENCFrame &frame) override;

    void OnFrame(const VENCFrame &frame) override;

private:
    bool init_;
};
} // namespace rs