#pragma once

#include "global.h"
#include "live/rtmp_live.h"
#include "record/mp4_record.h"

namespace rs
{

class Config
{
public:
    struct Video
    {
        int res_width;
        int res_height;
        int res_bitrate;

        int normal_live_width;
        int normal_live_height;
        int normal_live_bitrate;

        int normal_record_width;
        int normal_record_height;
        int normal_record_bitrate;

        operator Json::Value() const
        {
            Json::Value root;
            root["res_width"] = res_width;
            root["res_height"] = res_height;
            root["res_bitrate"] = res_bitrate;
            root["normal_live_width"] = normal_live_width;
            root["normal_live_height"] = normal_live_height;
            root["normal_live_bitrate"] = normal_live_bitrate;
            root["normal_record_width"] = normal_record_width;
            root["normal_record_height"] = normal_record_height;
            root["normal_record_bitrate"] = normal_record_bitrate;
            return root;
        }

        static bool IsOk(const Json::Value &root)
        {
            if (!root.isObject() ||
                !root.isMember("res_width") ||
                !root["res_width"].isInt() ||
                !root.isMember("res_height") ||
                !root["res_height"].isInt() ||
                !root.isMember("res_bitrate") ||
                !root["res_bitrate"].isInt() ||
                !root.isMember("normal_live_width") ||
                !root["normal_live_width"].isInt() ||
                !root.isMember("normal_live_height") ||
                !root["normal_live_height"].isInt() ||
                !root.isMember("normal_live_bitrate") ||
                !root["normal_live_bitrate"].isInt() ||
                !root.isMember("normal_record_width") ||
                !root["normal_record_width"].isInt() ||
                !root.isMember("normal_record_height") ||
                !root["normal_record_height"].isInt() ||
                !root.isMember("normal_record_bitrate") ||
                !root["normal_record_bitrate"].isInt())
                return false;

            return true;
        }

        Video &operator=(const Json::Value &root)
        {
            res_width = root["res_width"].asInt();
            res_height = root["res_height"].asInt();
            res_bitrate = root["res_bitrate"].asInt();
            normal_live_width = root["normal_live_width"].asInt();
            normal_live_height = root["normal_live_height"].asInt();
            normal_live_bitrate = root["normal_live_bitrate"].asInt();
            normal_record_width = root["normal_record_width"].asInt();
            normal_record_height = root["normal_record_height"].asInt();
            normal_record_bitrate = root["normal_record_bitrate"].asInt();
            return *this;
        }
    };

    struct Scene
    {
        enum Mode
        {
            NORMAL_MODE,
            PIP_MODE,
            TWO,
            THREE,
            FOUR,
            FOUR1,
            FIVE,
            SIX,
            SIX1
        };

        Mode mode;
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
            if (!root.isObject() ||
                !root.isMember("mode") ||
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

        Scene &operator=(const Json::Value &root)
        {
            mode = static_cast<Config::Scene::Mode>(root["mode"].asInt());
            Json::Value mapping_json = root["mapping"];
            for (size_t i = 0; i < mapping_json.size(); i++)
            {
                Json::Value item = mapping_json[i];
                Json::Value::Members members = item.getMemberNames();
                for (auto it = members.begin(); it != members.end(); it++)
                {
                    int no = std::stoi(*it);
                    RS_SCENE scene = static_cast<RS_SCENE>(item[*it].asInt());
                    mapping[no] = scene;
                }
            }
            return *this;
        }
    };

    struct System
    {
        struct Chn
        {
            int chn;
            RECT_S rect;

            operator Json::Value() const
            {
                Json::Value root;
                root["chn"] = chn;
                Json::Value rect_json;
                rect_json["x"] = rect.s32X;
                rect_json["y"] = rect.s32Y;
                rect_json["width"] = rect.u32Width;
                rect_json["height"] = rect.u32Height;
                root["rect"] = rect_json;
                return root;
            }

            static bool IsOk(const Json::Value &root)
            {
                if (!root.isObject() ||
                    !root.isMember("chn") ||
                    !root["chn"].isInt() ||
                    !root.isMember("rect") ||
                    !root["rect"].isObject())
                    return false;

                Json::Value rect_json = root["rect"];
                if (!rect_json.isMember("x") ||
                    !rect_json["x"].isInt() ||
                    !rect_json.isMember("y") ||
                    !rect_json["y"].isInt() ||
                    !rect_json.isMember("width") ||
                    !rect_json["width"].isUInt() ||
                    !rect_json.isMember("height") ||
                    !rect_json["height"].isUInt())
                    return false;

                return true;
            }

            Chn &operator=(const Json::Value &root)
            {
                chn = root["chn"].asInt();
                Json::Value rect_json = root["rect"];
                rect.s32X = rect_json["x"].asInt();
                rect.s32Y = rect_json["y"].asInt();
                rect.u32Width = rect_json["width"].asUInt();
                rect.u32Height = rect_json["height"].asUInt();
                return *this;
            }
        };

        VO_INTF_SYNC_E disp_vo_intf_sync;
        std::vector<Chn> chns;
        std::map<int, RS_SCENE> mapping;
        ADV7842_MODE pc_capture_mode;

        operator Json::Value() const
        {
            Json::Value root;
            root["disp_vo_intf_sync"] = disp_vo_intf_sync;

            Json::Value chns_json;
            for (const Chn &chn : chns)
                chns_json.append(chn);
            root["chns"] = chns_json;

            Json::Value mapping_json;
            for (auto it = mapping.begin(); it != mapping.end(); it++)
            {
                Json::Value item_json;
                item_json[std::to_string(it->first)] = it->second;
                mapping_json.append(item_json);
            }
            root["mapping"] = mapping_json;

            root["pc_capture_mode"] = pc_capture_mode;

            return root;
        }

        static bool IsOk(const Json::Value &root)
        {
            if (!root.isObject() ||
                !root.isMember("disp_vo_intf_sync") ||
                !root["disp_vo_intf_sync"].isInt() ||
                !root.isMember("chns") ||
                !root["chns"].isArray() ||
                !root.isMember("mapping") ||
                !root["mapping"].isArray() ||
                !root.isMember("pc_capture_mode") ||
                !root["pc_capture_mode"].isInt())
                return false;

            Json::Value chns_json = root["chns"];
            for (size_t i = 0; i < chns_json.size(); i++)
            {
                Json::Value item_json = chns_json[i];
                if (!Chn::IsOk(item_json))
                    return false;
            }

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

        System &operator=(const Json::Value &root)
        {
            disp_vo_intf_sync = static_cast<VO_INTF_SYNC_E>(root["disp_vo_intf_sync"].asInt());

            Json::Value chns_json = root["chns"];
            for (size_t i = 0; i < chns_json.size(); i++)
            {
                Chn chn;
                Json::Value item_json = chns_json[i];
                chn = item_json;
                chns.push_back(chn);
            }

            Json::Value mapping_json = root["mapping"];
            for (size_t i = 0; i < mapping_json.size(); i++)
            {
                Json::Value item = mapping_json[i];
                Json::Value::Members members = item.getMemberNames();
                for (auto it = members.begin(); it != members.end(); it++)
                {
                    int no = std::stoi(*it);
                    RS_SCENE scene = static_cast<RS_SCENE>(item[*it].asInt());
                    mapping[no] = scene;
                }
            }

            pc_capture_mode = static_cast<ADV7842_MODE>(root["pc_capture_mode"].asInt());

            return *this;
        }
    };

    struct Record
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

        Record &operator=(const Json::Value &root)
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

    struct LocalLive
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

        LocalLive &operator=(const Json::Value &root)
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

                    params.only_try_once = true;

                    lives.push_back(std::make_pair(scene, params));
                }
            }
            return *this;
        }

        std::vector<std::pair<RS_SCENE, rtmp::Params>> lives;
    };

    struct RemoteLive
    {
        operator Json::Value() const
        {
            return live;
        }

        static bool IsOk(const Json::Value &root)
        {
            if (!rtmp::Params::IsOk(root))
                return false;
            return true;
        }

        RemoteLive &operator=(const Json::Value &root)
        {
            live = root;
            live.has_audio = true;
            live.only_try_once = true;
            return *this;
        }

        rtmp::Params live;
    };

    static Config *Instance();

    virtual ~Config();

    int Initialize(const std::string &path);

    void Close();

    int WriteToFile();

    bool IsResourceMode();

    static bool IsResourceMode(Config::Scene::Mode mode);

protected:
    explicit Config();

public:
    Video video_;
    Scene scene_;
    System system_;
    RemoteLive remote_live_;
    LocalLive local_lives_;

private:
    std::string path_;
    bool init_;
};
} // namespace rs