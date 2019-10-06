#pragma once

#include "global.h"
#include "common/config.h"

namespace rs
{
struct ChangeOsdReq
{
    Config::Osd osd;

    operator Json::Value() const
    {
        return osd;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!Config::Osd::IsOk(root))
            return false;
        return true;
    }

    ChangeOsdReq &operator=(const Json::Value &root)
    {
        osd = root;
        return *this;
    }
};
} // namespace rs