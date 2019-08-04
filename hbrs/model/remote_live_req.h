#pragma once

#include "global.h"
#include "live/rtmp_live.h"
#include "common/config.h"

namespace rs
{

struct RemoteLiveReq
{
    operator Json::Value() const
    {
        return params;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!rtmp::Params::IsOk(root))
            return false;
        return true;
    }

    RemoteLiveReq &operator=(const Json::Value &root)
    {
        params = root;
        params.has_audio = true;
        params.only_try_once = true;
        return *this;
    }

    rtmp::Params params;
};
} // namespace rs