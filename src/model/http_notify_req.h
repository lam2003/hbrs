#pragma once

#include "global.h"

namespace rs
{
struct HttpNotifyReq
{
    enum Type
    {
        SIGNAL_STATUS = 0,
        ERROR_EVENT
    };

    Type type;
    std::string msg;

    operator Json::Value() const
    {
        Json::Value root;
        root["type"] = type;
        root["msg"] = msg;
        return root;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!root.isObject() ||
            !root.isMember("type") ||
            !root["type"].isInt() ||
            !root.isMember("msg") ||
            !root["msg"].isString())
            return false;
        return true;
    }

    HttpNotifyReq &operator=(const Json::Value &root)
    {
        type = static_cast<HttpNotifyReq::Type>(root["type"].asInt());
        msg = root["msg"].asString();
        return *this;
    }
};
} // namespace rs