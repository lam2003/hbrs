#pragma once

#include "global.h"
#include "live/rtmp_live.h"
#include "common/config.h"

namespace rs
{

struct LiveReq
{
    operator Json::Value() const
    {
        Json::Value root;
        for (const std::pair<RS_SCENE, rtmp::Params> &live : lives)
        {
            Json::Value live_json;
            live_json[std::to_string(live.first)] = live.second;
            root.append(live_json);
        }
        return root;
    }

    static bool IsOk(const Json::Value &root)
    {
        if (!root.isArray())
            return false;

        for (size_t i = 0; i < root.size(); i++)
        {
            Json::Value item = root[i];
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

                Json::Value params = item[*it];
                if (!params.isObject())
                    return false;
                if (!rtmp::Params::IsOk(params))
                    return false;
            }
        }
        return true;
    }

    LiveReq &operator=(const Json::Value &root)
    {
        for (size_t i = 0; i < root.size(); i++)
        {
            Json::Value item = root[i];
            Json::Value::Members members = item.getMemberNames();
            for (auto it = members.begin(); it != members.end(); it++)
            {
                RS_SCENE scene = static_cast<RS_SCENE>(std::stoi(*it));
                rtmp::Params params;
                params = item[*it];
                if (scene == MAIN || scene == MAIN2)
                {
                    params.has_audio = true;
                }
                else
                {
                    params.has_audio = false;
                }
                
                params.only_try_once = false;

                lives.push_back(std::make_pair(scene, params));
            }
        }
        return *this;
    }

    std::vector<std::pair<RS_SCENE, rtmp::Params>> lives;
};
} // namespace rs