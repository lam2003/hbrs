#pragma once

#include "common/config.h"

namespace rs
{
struct ChangeDisplayScreenReq
{
    Config::Display display;

    operator Json::Value() const
    {
        return display;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!Config::Display::IsOk(root))
            return false;
        return true;
    }

    ChangeDisplayScreenReq &operator=(const Json::Value &root)
    {
        display = root;
        return *this;
    }
};
} // namespace rs
