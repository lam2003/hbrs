#pragma once

#include "global.h"

namespace rs
{
struct ChangePCCaptureReq
{
    Config::Adv7842 adv7842;
    operator Json::Value() const
    {
        return adv7842;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!Config::Adv7842::IsOk(root))
            return false;

        return true;
    }

    ChangePCCaptureReq &operator=(const Json::Value &root)
    {
        adv7842 = root;
        return *this;
    }
};
} // namespace rs