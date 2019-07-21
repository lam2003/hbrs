#include "common/config.h"
#include "common/err_code.h"

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
        log_e("parse root failed");
        return KParamsError;
    }

    if (!root.isMember("video") ||
        !root["video"].isObject())
    {
        log_e("parse video failed");
        return KParamsError;
    }

    Json::Value video = root["video"];
    if (!video.isMember("record_width") ||
        !video["record_width"].isInt() ||
        !video.isMember("record_height") ||
        !video["record_height"].isInt() ||
        !video.isMember("record_bitrate") ||
        !video["record_bitrate"].isInt() ||
        !video.isMember("live_width") ||
        !video["live_width"].isInt() ||
        !video.isMember("live_height") ||
        !video["live_height"].isInt() ||
        !video.isMember("live_bitrate") ||
        !video["live_bitrate"].isInt())
    {
        log_e("check video error");
        return KParamsError;
    }

    video_.record_width = video["record_width"].asInt();
    video_.record_height = video["record_height"].asInt();
    video_.record_bitrate = video["record_bitrate"].asInt();
    video_.live_width = video["live_width"].asInt();
    video_.live_height = video["live_height"].asInt();
    video_.live_bitrate = video["live_bitrate"].asInt();
    
    {
        //scene
        if (!root.isMember("scene") ||
            !root["scene"].isObject())
        {
            log_e("parse scene failed");
            return KParamsError;
        }

        Json::Value scene = root["scene"];
        if (!scene.isMember("mode") ||
            !scene["mode"].isInt() ||
            !scene.isMember("mapping") ||
            !scene["mapping"].isArray())
        {
            log_e("check scene error");
            return KParamsError;
        }

        scene_.mode = static_cast<Scene::Mode>(scene["mode"].asInt());
        Json::Value mapping = scene["mapping"];
        for (size_t i = 0; i < mapping.size(); i++)
        {
            Json::Value item = mapping[i];
            if (!item.isObject())
                continue;
            Json::Value::Members members = item.getMemberNames();
            for (auto it = members.begin(); it != members.end(); it++)
            {
                try
                {
                    if (item[*it].isInt())
                        scene_.mapping[std::stoi(*it)] = static_cast<RS_SCENE>(item[*it].asInt());
                }
                catch (std::exception &e)
                {
                    continue;
                }
            }
        }
    }

    {
        //system
        Json::Value system = root["system"];
        if (!system.isMember("disp_vo_intf_sync") ||
            !system["disp_vo_intf_sync"].isInt() ||
            !system.isMember("chns") ||
            !system["chns"].isArray() ||
            !system.isMember("mapping") ||
            !system["mapping"].isArray() ||
            !system.isMember("pc_capture_mode") ||
            !system["pc_capture_mode"].isInt())
        {
            log_e("check system error");
            return KParamsError;
        }

        system_.disp_vo_intf_sync = static_cast<VO_INTF_SYNC_E>(system["disp_vo_intf_sync"].asInt());

        system_.chns.clear();
        Json::Value chns = system["chns"];
        for (size_t i = 0; i < chns.size(); i++)
        {
            Json::Value item = chns[i];
            if (!item.isObject())
                continue;

            if (!item.isMember("chn") ||
                !item["chn"].isInt() ||
                !item.isMember("rect") ||
                !item["rect"].isObject())
                continue;

            Json::Value rect = item["rect"];
            if (!rect.isMember("x") ||
                !rect["x"].isInt() ||
                !rect.isMember("y") ||
                !rect["y"].isInt() ||
                !rect.isMember("width") ||
                !rect["width"].isUInt() ||
                !rect.isMember("height") ||
                !rect["height"].isUInt())
                continue;

            system_.chns.push_back({item["chn"].asInt(),
                                    {rect["x"].asInt(),
                                     rect["y"].asInt(),
                                     rect["width"].asUInt(),
                                     rect["height"].asUInt()}});
        }

        Json::Value mapping = system["mapping"];
        for (size_t i = 0; i < mapping.size(); i++)
        {
            Json::Value item = mapping[i];
            if (!item.isObject())
                continue;
            Json::Value::Members members = item.getMemberNames();
            for (auto it = members.begin(); it != members.end(); it++)
            {
                try
                {
                    if (item[*it].isInt())
                        system_.mapping[std::stoi(*it)] = static_cast<RS_SCENE>(item[*it].asInt());
                }
                catch (std::exception &e)
                {
                    continue;
                }
            }
        }

        system_.pc_capture_mode = static_cast<ADV7842_MODE>(system["pc_capture_mode"].asInt());
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

int Config::WriteToFile()
{
    if (!init_)
        return KUnInitialized;

    // std::ofstream ofs(path_);

    // Json::Value root;

    // Json::Value video;
    // video["width"] = video_.width;
    // video["height"] = video_.height;
    // video["frame_rate"] = video_.frame_rate;
    // root["video"] = video;

    // Json::Value scene;
    // Json::Value normal;
    // normal["main_scene"] = scene_.normal.main_scene;
    // Json::Value pip;
    // pip["main_scene"] = scene_.pip.main_scene;
    // pip["minor_scene"] = scene_.pip.minor_scene;
    // Json::Value resource;
    // resource["mode"] = scene_.resource.mode;
    // resource["relations"] = Json::arrayValue;
    // for (size_t i = 0; i < scene_.resource.relations.size(); i++)
    //     resource["relations"].append(scene_.resource.relations[i]);
    // scene["normal"] = normal;
    // scene["pip"] = pip;
    // scene["resource"] = resource;
    // root["scene"] = scene;

    // ofs << root.toStyledString() << std::endl;
    // ofs.close();
    return KSuccess;
}

void Config::Close()
{
    if (!init_)
        return;

    WriteToFile();
    init_ = false;
}

} // namespace rs