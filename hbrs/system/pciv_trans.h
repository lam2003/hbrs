#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

#include "common/global.h"
#include "system/pciv_comm.h"

namespace rs
{
namespace pciv
{
struct Buffer
{
    uint32_t phy_addr[1];
    uint8_t *vir_addr;
};
} // namespace pciv
class PCIVTrans
{
public:
    virtual ~PCIVTrans();

    static PCIVTrans *Instance();

    int32_t Initialize(pciv::Context *ctx);

    void Close();

    void AddVideoSink(VideoSink<VDEC_STREAM_S> *video_sink);

    void RemoveAllVideoSink();

protected:
    static void UnpackAndSendStream(uint8_t *data, int32_t len, const std::vector<VideoSink<VDEC_STREAM_S> *> &video_sinks);

    explicit PCIVTrans();

private:
    std::mutex video_sinks_mux_;
    std::vector<VideoSink<VDEC_STREAM_S> *> video_sinks_;
    std::vector<pciv::Buffer> buffers_;
    std::vector<std::shared_ptr<std::thread>> threads_;
    std::atomic<bool> run_;
    pciv::Context *ctx_;
    bool init_;
};
}; // namespace rs