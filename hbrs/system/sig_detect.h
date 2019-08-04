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

    static SigDetect *Instance();

    int Initialize(pciv::Context *ctx, ADV7842_MODE mode);

    int SetPCCaptureMode(ADV7842_MODE mode);

    void Close();

    void AddVIFmtListener(VIFmtListener *listener);

    void RemoveAllVIFmtListener();

    void SetSignalStatusListener(SignalStatusListener *listener);

protected:
    explicit SigDetect();

private:
    std::mutex mux_;
    std::vector<VIFmtListener *> listeners_;
    std::vector<VideoInputFormat> fmts_;
    SignalStatusListener *status_listeners_;
    pciv::Context *ctx_;
    std::atomic<bool> run_;
    std::unique_ptr<std::thread> thread_;
    bool init_;
};
} // namespace rs