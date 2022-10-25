#pragma once

#include "common/switch.h"

namespace rs
{
struct CameraControlReq
{
    int camera_addr;
    SerialManager::Command cmd;
    int value;

    operator Json::Value() const
    {
        Json::Value root;
        root["camera_addr"] = camera_addr;
        root["cmd"] = cmd;
        root["value"] = value;

        return root;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!root.isObject() ||
            !root.isMember("camera_addr") ||
            !root["camera_addr"].isInt() ||
            !root.isMember("cmd") ||
            !root["cmd"].isInt() ||
            !root.isMember("value") ||
            !root["value"].isInt())
            return false;
        return true;
    }

    CameraControlReq &operator=(const Json::Value &root)
    {
        camera_addr = root["camera_addr"].asInt();
        cmd = static_cast<SerialManager::Command>(root["cmd"].asInt());
        value = root["value"].asInt();
        return *this;
    }
};
} // namespace rs