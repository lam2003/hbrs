#pragma once

#include "global.h"
#include "common/config.h"

namespace rs
{

struct RecordReq
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

    RecordReq &operator=(const Json::Value &root)
    {
        records = root;
        return *this;
    }
};
} // namespace rs