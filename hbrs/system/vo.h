#pragma once

//self
#include "global.h"
#include "common/video_define.h"

namespace rs
{
namespace vo
{
struct Params
{
    int32_t dev;
    VO_INTF_TYPE_E intf_type;
    VO_INTF_SYNC_E intf_sync;
};

// struct Channel
// {
//     RECT_S rect;
//     int32_t no;
//     int32_t level;
// };
} // namespace vo

class VideoOutput : public VideoRecver<VIDEO_FRAME_INFO_S>
{
public:
    explicit VideoOutput();

    virtual ~VideoOutput();

    int32_t Initialize(const vo::Params &params);

    void Close();

    int32_t StopAllChn();

    int SendFrame(int chn, VIDEO_FRAME_INFO_S &frame) override;

    int StartChannel(int chn, const RECT_S &rect, int level) override;

    int StopChannel(int chn) override;

protected:
    static int32_t StartHDMI(HI_HDMI_ID_E dev, VO_INTF_SYNC_E intf_sync);

    static int32_t StartDevLayer(int32_t dev, VO_INTF_TYPE_E intf_type, VO_INTF_SYNC_E intf_sync);

private:
    vo::Params params_;
    bool init_;

    static const HI_HDMI_ID_E HdmiDev;
};
} // namespace rs