#include "common/config.h"
#include "common/err_code.h"
#include "model/live_req.h"

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
        !root.isMember("system") ||
        !root["system"].isObject() ||
        !root.isMember("video") ||
        !root["video"].isObject())
    {
        log_e("check root format failed");
        return KParamsError;
    }

    if (!Config::Scene::IsOk(root["scene"]))
    {
        log_e("check scene format failed");
        return KParamsError;
    }
    if (!Config::System::IsOk(root["system"]))
    {
        log_e("check system format failed");
        return KParamsError;
    }
    if (!Config::Video::IsOk(root["video"]))
    {
        log_e("check video format failed");
        return KParamsError;
    }

    scene_ = root["scene"];
    system_ = root["system"];
    video_ = root["video"];

    if (root.isMember("local_lives") ||
        root["local_lives"].isArray())
    {
        if (LiveReq::IsOk(root["local_lives"]))
        {
            LiveReq req;
            Json::Value local_lives_json = root["local_lives"];
            req = local_lives_json;
            local_lives_ = req.lives;
        }
    }

    if (root.isMember("remote_live") ||
        root["remote_live"].isObject())
    {
        if (rtmp::Params::IsOk(root["remote_live"]))
        {
            Json::Value remote_live_json = root["remote_live"];
            remote_live_ = remote_live_json;
            remote_live_.has_audio = true;
            remote_live_.only_try_once = true;
        }
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
    root["system"] = system_;

    if (remote_live_.url != "")
        root["remote_live"] = remote_live_;

    if (!local_lives_.empty())
    {
        Json::Value local_lives_json;
        for (const std::pair<RS_SCENE, rtmp::Params> live : local_lives_)
        {
            Json::Value item_json;
            item_json[std::to_string(live.first)] = live.second;
            local_lives_json.append(item_json);
        }

        root["local_lives"] = local_lives_json;
    }
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

} // namespace rs