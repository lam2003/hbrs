#pragma once

#include "global.h"
#include "common/config.h"

namespace rs
{
struct ChangeRecordModeReq
{
    Config::RecordMode record_mode;

    operator Json::Value() const
    {
        return record_mode;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!Config::RecordMode::IsOk(root))
            return false;
        return true;
    }

    ChangeRecordModeReq &operator=(const Json::Value &root)
    {
        record_mode = root;
        return *this;
    }
};

} // namespace rs