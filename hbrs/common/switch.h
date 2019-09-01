#pragma once

#include "global.h"

namespace rs
{
class SwitchEventListener
{
public:
    virtual ~SwitchEventListener() {}

    virtual void OnSwitchEvent(RS_SCENE scene) = 0;
};
class SerialManager
{
public:
    explicit SerialManager();

    virtual ~SerialManager();

    int Initialize(event_base *base);

    void Close();

    void OnRead(evutil_socket_t socket);

    void SetEventListener(std::shared_ptr<SwitchEventListener> listener);

private:
    uint8_t buf[1024];
    int fd_;
    event *ev_;
    std::shared_ptr<SwitchEventListener> listener_;
    bool init_;
};
} // namespace rs