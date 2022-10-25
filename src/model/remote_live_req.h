#pragma once

#include "global.h"
#include "live/rtmp_live.h"
#include "common/config.h"

namespace rs
{

struct RemoteLiveReq
{
    Config::RemoteLive remote_live;

    operator Json::Value() const
    {
        return remote_live;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!rtmp::Params::IsOk(root))
            return false;
        return true;
    }

    RemoteLiveReq &operator=(const Json::Value &root)
    {
        remote_live = root;
        return *this;
    }
};
} // namespace rs