#pragma once

#include "global.h"
#include "common/config.h"

namespace rs
{
struct ChangeOsdTsReq
{
    Config::OsdTs osd_ts;

    operator Json::Value() const
    {
        return osd_ts;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!Config::OsdTs::IsOk(root))
            return false;
        return true;
    }

    ChangeOsdTsReq &operator=(const Json::Value &root)
    {
        osd_ts = root;
        return *this;
    }
};
} // namespace rs