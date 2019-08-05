#pragma once

#include "global.h"
#include "live/rtmp_live.h"
#include "common/config.h"

namespace rs
{

struct LocalLiveReq
{
    Config::LocalLive local_lives;

    operator Json::Value() const
    {
        return local_lives;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!Config::LocalLive::IsOk(root))
            return false;
        return true;
    }

    LocalLiveReq &operator=(const Json::Value &root)
    {
        local_lives = root;
        return *this;
    }
};
} // namespace rs