#pragma once

//self
#include "system/pciv_comm.h"

namespace rs
{

struct PCIVBuffer
{
    uint32_t phy_addr[1];
    uint8_t *vir_addr;
};

class PCIVTrans
{
public:
    virtual ~PCIVTrans();

    static PCIVTrans *Instance();

    int32_t Initialize(pciv::Context *ctx);

    void Close();

    void AddVideoSink(VideoSink<VDEC_STREAM_S> *sink);

    void RemoveAllVideoSink();

protected:
    static void UnpackAndSendStream(uint8_t *data, int32_t len, const std::vector<VideoSink<VDEC_STREAM_S> *> &video_sinks);

    explicit PCIVTrans();

private:
    std::mutex mux_;
    std::vector<VideoSink<VDEC_STREAM_S> *> sinks_;
    std::vector<PCIVBuffer> bufs_;
    std::vector<std::shared_ptr<std::thread>> threads_;

    std::atomic<bool> run_;
    pciv::Context *ctx_;
    bool init_;
};
}; // namespace rs