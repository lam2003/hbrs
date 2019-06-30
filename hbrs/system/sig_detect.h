//stl
#include <thread>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
//self
#include "common/global.h"
#include "system/pciv_comm.h"

namespace rs
{
class SigDetect
{
public:
    virtual ~SigDetect();

    static SigDetect *Instance();

    int Initialize(pciv::Context *ctx);

    void Close();

protected:
    explicit SigDetect();

private:
    std::mutex mux_;
    std::vector<VideoInputFormat> fmts_;
    pciv::Context *ctx_;
    std::atomic<bool> run_;
    std::unique_ptr<std::thread> thread_;
    bool init_;
};
} // namespace rs