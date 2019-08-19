#pragma once

#include "global.h"

namespace rs
{

class Switch
{
public:
    explicit Switch();

    virtual ~Switch();

    int Initialize(event_base *base);

    void Close();

    void OnRead(evutil_socket_t socket);

private:
    uint8_t buf[1024];
    int fd_;
    event *ev_;
    bool init_;
};
} // namespace rs