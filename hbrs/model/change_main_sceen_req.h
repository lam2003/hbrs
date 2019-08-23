#pragma once

#include "global.h"
#include "common/config.h"

namespace rs
{
struct ChangeMainScreenReq
{
    Config::Scene scene;

    operator Json::Value() const
    {
        return scene;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!Config::Scene::IsOk(root))
            return false;
        return true;
    }

    ChangeMainScreenReq &operator=(const Json::Value &root)
    {
        scene = root;
        return *this;
    }
};
} // namespace rs