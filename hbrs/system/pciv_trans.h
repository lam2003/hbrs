#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

#include "common/global.h"
#include "common/buffer.h"
#include "system/pciv_comm.h"

namespace rs
{

class PCIVTrans
{
public:
    virtual ~PCIVTrans();

    static PCIVTrans *Instance();

    int Initialize(pciv::Context *ctx);

    void Close();

protected:
    explicit PCIVTrans();

private:
    std::vector<std::shared_ptr<std::thread>> threads_;
    std::atomic<bool> run_;
    pciv::Context *ctx_;
    bool init_;
};
}; // namespace rs