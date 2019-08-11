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

        operator Json::Value() const;
        static bool IsOk(const Json::Value &root);
        Video &operator=(const Json::Value &root);
    };

    struct Adv7842
    {
        ADV7842_MODE pc_capture_mode;
        operator Json::Value() const;
        static bool IsOk(const Json::Value &root);
        Adv7842 &operator=(const Json::Value &root);
    };

    struct Display
    {
        VO_INTF_SYNC_E disp_vo_intf_sync;
        std::map<int, RECT_S> display_pos;
        std::map<int, RS_SCENE> mapping;

        operator Json::Value() const;
        static bool IsOk(const Json::Value &root);
        Display &operator=(const Json::Value &root);
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

        operator Json::Value() const;
        static bool IsOk(const Json::Value &root);
        Scene &operator=(const Json::Value &root);
    };

    struct ResourceRecord
    {
        std::vector<std::pair<RS_SCENE, mp4::Params>> records;

        operator Json::Value() const;
        static bool IsOk(const Json::Value &root);
        ResourceRecord &operator=(const Json::Value &root);
    };

    struct NormalRecord
    {
        mp4::Params record;

        operator Json::Value() const;
        static bool IsOk(const Json::Value &root);
        NormalRecord &operator=(const Json::Value &root);
    };

    struct LocalLive
    {
        std::vector<std::pair<RS_SCENE, rtmp::Params>> lives;

        operator Json::Value() const;
        static bool IsOk(const Json::Value &root);
        LocalLive &operator=(const Json::Value &root);
    };

    struct RemoteLive
    {
        rtmp::Params live;

        operator Json::Value() const;
        static bool IsOk(const Json::Value &root);
        RemoteLive &operator=(const Json::Value &root);
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
    Adv7842 adv7842_;
    Scene scene_;
    Display display_;
    RemoteLive remote_live_;
    LocalLive local_lives_;
    ResourceRecord resource_records_;
    NormalRecord normal_record_;
private:
    std::string path_;
    bool init_;
};
} // namespace rs