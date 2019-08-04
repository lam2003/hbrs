#pragma once

#include "global.h"
#include "common/config.h"

namespace rs
{
struct ChangeMainScreenReq
{
    Config::Scene::Mode mode;
    std::map<int, RS_SCENE> mapping;

    operator Json::Value() const
    {
        Json::Value root;
        root["mode"] = mode;

        Json::Value mapping_json;
        for (auto it = mapping.begin(); it != mapping.end(); it++)
        {
            Json::Value item_json;
            item_json[std::to_string(it->first)] = it->second;
            mapping_json.append(item_json);
        }
        root["mapping"] = mapping_json;
        return root;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!root.isMember("mode") ||
            !root["mode"].isInt() ||
            !root.isMember("mapping") ||
            !root["mapping"].isArray())
            return false;
        Json::Value mapping_json = root["mapping"];
        for (size_t i = 0; i < mapping_json.size(); i++)
        {
            Json::Value item = mapping_json[i];
            if (!item.isObject())
                return false;
            Json::Value::Members members = item.getMemberNames();
            for (auto it = members.begin(); it != members.end(); it++)
            {
                try
                {
                    std::stoi(*it);
                }
                catch (...)
                {
                    return false;
                }
                if (!item[*it].isInt())
                    return false;
            }
        }

        return true;
    }

    ChangeMainScreenReq &operator=(const Json::Value &root)
    {
        mode = static_cast<Config::Scene::Mode>(root["mode"].asInt());
        Json::Value mapping_json = root["mapping"];
        for (size_t i = 0; i < mapping_json.size(); i++)
        {
            Json::Value item = mapping_json[i];
            Json::Value::Members members = item.getMemberNames();
            for (auto it = members.begin(); it != members.end(); it++)
            {
                int no= std::stoi(*it);
                RS_SCENE scene = static_cast<RS_SCENE>(item[*it].asInt());
                mapping[no] = scene;
            }
        }
        return *this;
    }
};
} // namespace rs