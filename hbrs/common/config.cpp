#include "common/config.h"
#include "common/err_code.h"
#include "common/utils.h"

namespace rs
{

Config::Config() : path_("./test.json"),
                   init_(false)
{
}

Config::~Config()
{
    Close();
}

Config *Config::Instance()
{
    static Config *instance = new Config;
    return instance;
}

int Config::Initialize(const std::string &path)
{
    if (init_)
        return KInitialized;

    path_ = path;
    std::ifstream ifs(path_);
    if (!ifs)
    {
        log_e("open %s failed,%s", path_.c_str(), strerror(errno));
        return KParamsError;
    }

    Json::Reader reader(Json::Features::strictMode());
    Json::Value root;
    if (!reader.parse(ifs, root))
    {
        log_e("parse json root failed");
        return KParamsError;
    }

    if (!root.isMember("scene") ||
        !root["scene"].isObject() ||
        !root.isMember("display") ||
        !root["display"].isObject() ||
        !root.isMember("video") ||
        !root["video"].isObject() ||
        !root.isMember("adv7842") ||
        !root["adv7842"].isObject() ||
        !root.isMember("switch_cmd") ||
        !root["switch_cmd"].isObject())
    {
        log_e("check root format failed");
        return KParamsError;
    }

    if (!Config::Scene::IsOk(root["scene"]))
    {
        log_e("check scene format failed");
        return KParamsError;
    }
    if (!Config::Display::IsOk(root["display"]))
    {
        log_e("check display format failed");
        return KParamsError;
    }
    if (!Config::Video::IsOk(root["video"]))
    {
        log_e("check video format failed");
        return KParamsError;
    }
    if (!Config::Adv7842::IsOk(root["adv7842"]))
    {
        log_e("check adv7842 format failed");
        return KParamsError;
    }
    if (!Config::SwitchCommand::IsOk(root["switch_cmd"]))
    {
        log_e("check switch_cmd format failed");
        return KParamsError;
    }

    scene_ = root["scene"];
    display_ = root["display"];
    video_ = root["video"];
    adv7842_ = root["adv7842"];
    switch_cmd_ = root["switch_cmd"];

    if (root.isMember("local_lives"))
    {
        if (Config::LocalLive::IsOk(root["local_lives"]))
            local_lives_ = root["local_lives"];
    }

    if (root.isMember("remote_live"))
    {
        if (Config::RemoteLive::IsOk(root["remote_live"]))
            remote_live_ = root["remote_live"];
    }

    init_ = true;
    return KSuccess;
}

bool Config::IsResourceMode()
{
    if (!init_)
        return false;

    switch (scene_.mode)
    {
    case Scene::Mode::NORMAL_MODE:
    case Scene::Mode::PIP_MODE:
        return false;
    case Scene::Mode::TWO:
    case Scene::Mode::THREE:
    case Scene::Mode::FOUR:
    case Scene::Mode::FOUR1:
    case Scene::Mode::FIVE:
    case Scene::Mode::SIX:
    case Scene::Mode::SIX1:
        return true;

    default:
        RS_ASSERT(0);
    }
}

bool Config::IsResourceMode(Config::Scene::Mode mode)
{
    switch (mode)
    {
    case Scene::Mode::NORMAL_MODE:
    case Scene::Mode::PIP_MODE:
        return false;
    case Scene::Mode::TWO:
    case Scene::Mode::THREE:
    case Scene::Mode::FOUR:
    case Scene::Mode::FOUR1:
    case Scene::Mode::FIVE:
    case Scene::Mode::SIX:
    case Scene::Mode::SIX1:
        return true;

    default:
        RS_ASSERT(0);
    }
}

int Config::WriteToFile()
{
    if (!init_)
        return KUnInitialized;

    std::ofstream ofs(path_);
    Json::Value root;

    root["video"] = video_;
    root["scene"] = scene_;
    root["display"] = display_;
    root["adv7842"] = adv7842_;
    root["switch_cmd"] = switch_cmd_;

    if (remote_live_.live.url != "")
        root["remote_live"] = remote_live_;

    if (!local_lives_.lives.empty())
        root["local_lives"] = local_lives_;

    ofs << root.toStyledString() << std::endl;
    ofs.close();
    return KSuccess;
}

void Config::Close()
{
    if (!init_)
        return;

    WriteToFile();
    init_ = false;
}

Config::Video::operator Json::Value() const
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

bool Config::Video::IsOk(const Json::Value &root)
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

Config::Video &Config::Video::operator=(const Json::Value &root)
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

Config::Adv7842::operator Json::Value() const
{
    Json::Value root;
    root["pc_capture_mode"] = pc_capture_mode;
    return root;
}

bool Config::Adv7842::IsOk(const Json::Value &root)
{
    if (!root.isObject() ||
        !root.isMember("pc_capture_mode") ||
        !root["pc_capture_mode"].isInt())
        return false;
    return true;
}

Config::Adv7842 &Config::Adv7842::operator=(const Json::Value &root)
{
    pc_capture_mode = static_cast<ADV7842_MODE>(root["pc_capture_mode"].asInt());
    return *this;
}

Config::Display::operator Json::Value() const
{
    Json::Value root;
    root["disp_vo_intf_sync"] = disp_vo_intf_sync;

    Json::Value display_pos_json;
    for (auto it = display_pos.begin(); it != display_pos.end(); it++)
    {
        Json::Value item_json;

        Json::Value rect_json;
        rect_json["x"] = it->second.s32X;
        rect_json["y"] = it->second.s32Y;
        rect_json["width"] = it->second.u32Width;
        rect_json["height"] = it->second.u32Height;
        item_json[std::to_string(it->first)] = rect_json;

        display_pos_json.append(item_json);
    }
    root["display_pos"] = display_pos_json;

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

bool Config::Display::IsOk(const Json::Value &root)
{
    if (!root.isObject() ||
        !root.isMember("disp_vo_intf_sync") ||
        !root["disp_vo_intf_sync"].isInt() ||
        !root.isMember("display_pos") ||
        !root["display_pos"].isArray() ||
        !root.isMember("mapping") ||
        !root["mapping"].isArray())
        return false;

    Json::Value display_pos_json = root["display_pos"];
    for (size_t i = 0; i < display_pos_json.size(); i++)
    {
        Json::Value item_json = display_pos_json[i];
        if (!item_json.isObject())
            return false;
        Json::Value::Members members = item_json.getMemberNames();
        for (auto it = members.begin(); it != members.end(); it++)
        {
            Json::Value::Members members = item_json.getMemberNames();
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
                if (!item_json[*it].isObject())
                    return false;
                Json::Value rect_json = item_json[*it];
                if (!rect_json.isMember("x") ||
                    !rect_json["x"].isInt() ||
                    !rect_json.isMember("y") ||
                    !rect_json["y"].isInt() ||
                    !rect_json.isMember("width") ||
                    !rect_json["width"].isUInt() ||
                    !rect_json.isMember("height") ||
                    !rect_json["height"].isUInt())
                    return false;
            }
        }
    }

    Json::Value mapping_json = root["mapping"];
    for (size_t i = 0; i < mapping_json.size(); i++)
    {
        Json::Value item_json = mapping_json[i];
        if (!item_json.isObject())
            return false;
        Json::Value::Members members = item_json.getMemberNames();
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
            if (!item_json[*it].isInt())
                return false;
        }
    }
    return true;
}

Config::Display &Config::Display::operator=(const Json::Value &root)
{
    disp_vo_intf_sync = static_cast<VO_INTF_SYNC_E>(root["disp_vo_intf_sync"].asInt());
    Json::Value display_pos_json = root["display_pos"];
    for (size_t i = 0; i < display_pos_json.size(); i++)
    {
        Json::Value item_json = display_pos_json[i];
        Json::Value::Members members = item_json.getMemberNames();
        for (auto it = members.begin(); it != members.end(); it++)
        {
            int no = std::stoi(*it);

            Json::Value rect_json = item_json[*it];
            RECT_S rect;
            rect.s32X = rect_json["x"].asInt();
            rect.s32Y = rect_json["y"].asInt();
            rect.u32Width = rect_json["width"].asUInt();
            rect.u32Height = rect_json["height"].asUInt();
            display_pos[no] = rect;
        }
    }

    Json::Value mapping_json = root["mapping"];
    for (size_t i = 0; i < mapping_json.size(); i++)
    {
        Json::Value item_json = mapping_json[i];
        Json::Value::Members members = item_json.getMemberNames();
        for (auto it = members.begin(); it != members.end(); it++)
        {
            int no = std::stoi(*it);
            RS_SCENE scene = static_cast<RS_SCENE>(item_json[*it].asInt());
            mapping[no] = scene;
        }
    }
    return *this;
}

Config::Scene::operator Json::Value() const
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

bool Config::Scene::IsOk(const Json::Value &root)
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
        Json::Value item_json = mapping_json[i];
        if (!item_json.isObject())
            return false;
        Json::Value::Members members = item_json.getMemberNames();
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
            if (!item_json[*it].isInt())
                return false;
        }
    }
    return true;
}

Config::Scene &Config::Scene::operator=(const Json::Value &root)
{
    mode = static_cast<Config::Scene::Mode>(root["mode"].asInt());
    Json::Value mapping_json = root["mapping"];
    for (size_t i = 0; i < mapping_json.size(); i++)
    {
        Json::Value item_json = mapping_json[i];
        Json::Value::Members members = item_json.getMemberNames();
        for (auto it = members.begin(); it != members.end(); it++)
        {
            int no = std::stoi(*it);
            RS_SCENE scene = static_cast<RS_SCENE>(item_json[*it].asInt());
            mapping[no] = scene;
        }
    }
    return *this;
}

Config::Record::operator Json::Value() const
{
    Json::Value root;
    for (const std::pair<RS_SCENE, mp4::Params> &record : records)
    {
        Json::Value rec_json;
        rec_json[std::to_string(record.first)] = record.second;
        root.append(rec_json);
    }
    return root;
}

bool Config::Record::IsOk(const Json::Value &root)
{
    if (!root.isArray())
        return false;

    for (size_t i = 0; i < root.size(); i++)
    {
        Json::Value item_json = root[i];
        if (!item_json.isObject())
            return false;

        Json::Value::Members members = item_json.getMemberNames();
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

            Json::Value params = item_json[*it];
            if (!params.isObject())
                return false;
            if (!mp4::Params::IsOk(params))
                return false;
        }
    }
    return true;
}

Config::Record &Config::Record::operator=(const Json::Value &root)
{
    for (size_t i = 0; i < root.size(); i++)
    {
        Json::Value item_json = root[i];
        Json::Value::Members members = item_json.getMemberNames();
        for (auto it = members.begin(); it != members.end(); it++)
        {
            RS_SCENE scene = static_cast<RS_SCENE>(std::stoi(*it));
            mp4::Params params;
            params = item_json[*it];
            params.frame_rate = 25;
            params.samplate_rate = 48000;
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
            records.push_back(std::make_pair(scene, params));
        }
    }
    return *this;
}


Config::LocalLive::operator Json::Value() const
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

bool Config::LocalLive::IsOk(const Json::Value &root)
{
    if (!root.isArray())
        return false;

    for (size_t i = 0; i < root.size(); i++)
    {
        Json::Value item_json = root[i];
        if (!item_json.isObject())
            return false;

        Json::Value::Members members = item_json.getMemberNames();
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

            Json::Value params = item_json[*it];
            if (!params.isObject())
                return false;
            if (!rtmp::Params::IsOk(params))
                return false;
        }
    }
    return true;
}

Config::LocalLive &Config::LocalLive::operator=(const Json::Value &root)
{
    for (size_t i = 0; i < root.size(); i++)
    {
        Json::Value item_json = root[i];
        Json::Value::Members members = item_json.getMemberNames();
        for (auto it = members.begin(); it != members.end(); it++)
        {
            RS_SCENE scene = static_cast<RS_SCENE>(std::stoi(*it));
            rtmp::Params params;
            params = item_json[*it];
            params.only_try_once = true;
            params.has_audio = false;
            if (scene == MAIN || scene == MAIN2)
                params.has_audio = true;
            lives.push_back(std::make_pair(scene, params));
        }
    }
    return *this;
}

Config::RemoteLive::operator Json::Value() const
{
    return live;
}

bool Config::RemoteLive::IsOk(const Json::Value &root)
{
    if (!rtmp::Params::IsOk(root))
        return false;
    return true;
}

Config::RemoteLive &Config::RemoteLive::operator=(const Json::Value &root)
{
    live = root;
    live.has_audio = true;
    live.only_try_once = true;
    return *this;
}

Config::SwitchCommand::operator Json::Value() const
{
    std::string str;
    Json::Value root;
    if (Utils::HexInt2String(tea_fea, str))
        root["tea_fea"] = str;
    if (Utils::HexInt2String(stu_fea, str))
        root["stu_fea"] = str;
    if (Utils::HexInt2String(tea_full, str))
        root["tea_full"] = str;
    if (Utils::HexInt2String(stu_full, str))
        root["stu_full"] = str;
    if (Utils::HexInt2String(bb_fea, str))
        root["bb_fea"] = str;
    if (Utils::HexInt2String(pc_capture, str))
        root["pc_capture"] = str;
    return root;
}

bool Config::SwitchCommand::IsOk(const Json::Value &root)
{
    if (!root.isObject() ||
        !root.isMember("tea_fea") ||
        !root["tea_fea"].isString() ||
        !root.isMember("stu_fea") ||
        !root["stu_fea"].isString() ||
        !root.isMember("tea_full") ||
        !root["tea_full"].isString() ||
        !root.isMember("stu_full") ||
        !root["stu_full"].isString() ||
        !root.isMember("bb_fea") ||
        !root["bb_fea"].isString() ||
        !root.isMember("pc_capture") ||
        !root["pc_capture"].isString())
        return false;
    return true;
}

Config::SwitchCommand &Config::SwitchCommand::operator=(const Json::Value &root)
{
    std::vector<int> hex_int_arr;
    if (Utils::HexString2Int(root["tea_fea"].asString(), hex_int_arr))
        tea_fea = hex_int_arr;
    hex_int_arr.clear();
    if (Utils::HexString2Int(root["stu_fea"].asString(), hex_int_arr))
        stu_fea = hex_int_arr;
    hex_int_arr.clear();
    if (Utils::HexString2Int(root["tea_full"].asString(), hex_int_arr))
        tea_full = hex_int_arr;
    hex_int_arr.clear();
    if (Utils::HexString2Int(root["stu_full"].asString(), hex_int_arr))
        stu_full = hex_int_arr;
    hex_int_arr.clear();
    if (Utils::HexString2Int(root["bb_fea"].asString(), hex_int_arr))
        bb_fea = hex_int_arr;
    hex_int_arr.clear();
    if (Utils::HexString2Int(root["pc_capture"].asString(), hex_int_arr))
        pc_capture = hex_int_arr;

    return *this;
}

} // namespace rs