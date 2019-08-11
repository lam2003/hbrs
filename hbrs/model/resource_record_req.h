#pragma once

#include "global.h"
#include "common/config.h"

namespace rs
{

struct ResourceRecordReq
{
    Config::ResourceRecord records;

    operator Json::Value() const
    {
        return records;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!Config::ResourceRecord::IsOk(root))
            return false;
        return true;
    }

    ResourceRecordReq &operator=(const Json::Value &root)
    {
        records = root;
        return *this;
    }
};
} // namespace rs