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
    enum Command
    {
        RESET = 0,      //0
        STOP,           //1
        UP,             //2
        DOWN,           //3
        LEFT,           //4
        RIGHT,          //5
        ZOOM,           //6
        SET_ZOOM_SPEED, //7
        SET_MEMORY,     //8
        LOAD_MEMORY,    //9
        DEL_MEMORY      //10
    };

    explicit SerialManager();

    virtual ~SerialManager();

    int Initialize(event_base *base);

    void Close();

    void OnRead(evutil_socket_t socket);

    void SetEventListener(std::shared_ptr<SwitchEventListener> listener);

    int CameraControl(int camera_addr, SerialManager::Command cmd, int value);

private:
    uint8_t buf[1024];
#if 1
    VISCACamera_t camera_;
    VISCAInterface_t interface_;
#else
    int fd_;
#endif
    event *ev_;
    std::shared_ptr<SwitchEventListener> listener_;
    bool init_;
};
} // namespace rs