#pragma once

#include "global.h"

namespace rs
{
struct SwitchReq
{
    RS_SCENE scene;

    operator Json::Value() const
    {
        Json::Value root;
        root["scene"] = scene;
        return root;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!root.isObject() ||
            !root.isMember("scene") ||
            !root["scene"].isInt())
            return false;

        return true;
    }

    SwitchReq &operator=(const Json::Value &root)
    {
        scene = static_cast<RS_SCENE>(root["scene"].asInt());
        return *this;
    }
};
} // namespace rs