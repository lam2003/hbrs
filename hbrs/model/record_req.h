#pragma once

#include "global.h"
#include "common/config.h"

namespace rs
{

struct RecordReq
{
    Config::Record records;

    operator Json::Value() const
    {
        return records;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!Config::Record::IsOk(root))
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