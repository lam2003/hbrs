#pragma once
//self
#include "system/pciv_comm.h"

namespace rs
{

class SignalStatusListener
{
public:
    virtual ~SignalStatusListener() {}
    virtual void OnUpdate(const std::vector<VideoInputFormat> &fmts) = 0;
};

class SigDetect
{
public:
    virtual ~SigDetect();

    explicit SigDetect();

    int Initialize(std::shared_ptr<PCIVComm> pciv_comm, ADV7842_MODE mode);

    int SetPCCaptureMode(ADV7842_MODE mode);

    void Close();

    void AddVIFmtListener(std::shared_ptr<VIFmtListener> listener);

    void RemoveAllVIFmtListener();

    void SetSignalStatusListener(SignalStatusListener *listener);

private:
    std::mutex mux_;
    std::vector<std::shared_ptr<VIFmtListener>> listeners_;
    std::vector<VideoInputFormat> fmts_;
    SignalStatusListener *status_listeners_;
    std::shared_ptr<PCIVComm> pciv_comm_;
    std::atomic<bool> run_;
    std::unique_ptr<std::thread> thread_;
    bool init_;
};
} // namespace rs