#pragma once

#include "global.h"

class JsonUtils
{
public:
    static std::string toStr(const Json::Value &json)
    {
        Json::FastWriter w;
        std::string value = w.write(json);
        return value.substr(0, value.size() - 1);
    }

    static int toJson(const std::string &str, Json::Value &root)
    {
        if (str.empty())
            return 1;
        Json::Reader reader(Json::Features::strictMode());
        if (!reader.parse(str, root))
            return 1;
        return 0;
    }
};

