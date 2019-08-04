#pragma once

#include "global.h"

namespace rs
{
struct ChangePCCaptureReq
{
    ADV7842_MODE mode;
    operator Json::Value() const
    {
        Json::Value root;
        root["mode"] = mode;
        return root;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!root.isMember("mode") ||
            !root["mode"].isInt())
            return false;
        return true;
    }

    ChangePCCaptureReq &operator=(const Json::Value &root)
    {
        mode = static_cast<ADV7842_MODE>(root["mode"].asInt());
        return *this;
    }
};
} // namespace rs