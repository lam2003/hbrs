#pragma once

#include "global.h"
#include "record/mp4_record.h"
#include "common/config.h"

namespace rs
{

struct RecordReq
{
    operator Json::Value() const
    {
        Json::Value root;
        for (const std::pair<RS_SCENE, mp4::Params> &rec : recs)
        {
            Json::Value rec_json;
            rec_json[std::to_string(rec.first)] = rec.second;
            root.append(rec_json);
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
                if (!mp4::Params::IsOk(params))
                    return false;
            }
        }
        return true;
    }

    RecordReq &operator=(const Json::Value &root)
    {
        for (size_t i = 0; i < root.size(); i++)
        {
            Json::Value item = root[i];
            Json::Value::Members members = item.getMemberNames();
            for (auto it = members.begin(); it != members.end(); it++)
            {
                RS_SCENE scene = static_cast<RS_SCENE>(std::stoi(*it));
                mp4::Params params;
                params = item[*it];

                if (Config::Instance()->IsResourceMode())
                {
                    params.width = Config::Instance()->video_.res_width;
                    params.height = Config::Instance()->video_.res_height;
                }
                else
                {
                    params.width = Config::Instance()->video_.normal_record_width;
                    params.height = Config::Instance()->video_.normal_record_height;
                }
                params.frame_rate = 25;
                params.samplate_rate = 44100;
                recs.push_back(std::make_pair(scene, params));
            }
        }
        return *this;
    }

    std::vector<std::pair<RS_SCENE, mp4::Params>> recs;
};
} // namespace rs