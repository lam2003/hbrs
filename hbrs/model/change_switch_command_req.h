#pragma once

#include "common/config.h"

namespace rs
{
struct ChangeSwitchCommandReq
{
    Config::SwitchCommand commands;
    operator Json::Value() const
    {
        return commands;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!Config::SwitchCommand::IsOk(root))
            return false;
        return true;
    }

    ChangeSwitchCommandReq &operator=(const Json::Value &root)
    {
        commands = root;
        return *this;
    }
};
} // namespace rs