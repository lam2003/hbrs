#pragma once

#include "common/config.h"

namespace rs
{
struct ChangeVideoReq
{
    Config::Video video;

    operator Json::Value() const
    {
        return video;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!Config::Video::IsOk(root))
            return false;

        return true;
    }

    ChangeVideoReq &operator=(const Json::Value &root)
    {
        video = root;
        return *this;
    }
};
} // namespace rs