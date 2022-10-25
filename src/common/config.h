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

    struct Record
    {
        std::vector<std::pair<RS_SCENE, mp4::Params>> records;

        operator Json::Value() const;
        static bool IsOk(const Json::Value &root);
        Record &operator=(const Json::Value &root);
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

    struct SwitchCommand
    {
        std::vector<int> tea_fea;
        std::vector<int> stu_fea;
        std::vector<int> tea_full;
        std::vector<int> stu_full;
        std::vector<int> bb_fea;
        std::vector<int> pc_capture;

        operator Json::Value() const;
        static bool IsOk(const Json::Value &root);
        SwitchCommand &operator=(const Json::Value &root);
    };

    struct Web
    {
        std::string ip;
        int port;
        std::string path;
        operator Json::Value() const;
        static bool IsOk(const Json::Value &root);
        Web &operator=(const Json::Value &root);
    };

    struct Logger
    {
        std::string dir_path;
        bool auto_clean;
        int clean_duration;

        operator Json::Value() const;
        static bool IsOk(const Json::Value &root);
        Logger &operator=(const Json::Value &root);
    };
    struct OsdTs
    {
        std::string font_file;
        int font_size;
        uint8_t color_r;
        uint8_t color_g;
        uint8_t color_b;
        int x;
        int y;
        std::string time_format;
        bool add_ts;
        operator Json::Value() const;
        static bool IsOk(const Json::Value &root);
        OsdTs &operator=(const Json::Value &root);
    };
    struct Osd
    {
        struct Item
        {
            std::string font_file;
            int font_size;
            uint8_t color_r;
            uint8_t color_g;
            uint8_t color_b;
            int x;
            int y;
            std::string content;
            operator Json::Value() const;
            static bool IsOk(const Json::Value &root);
            Item &operator=(const Json::Value &root);
        };
        std::vector<Item> items;
        operator Json::Value() const;
        static bool IsOk(const Json::Value &root);
        Osd &operator=(const Json::Value &root);
    };
    struct RecordMode
    {
        bool is_resource_mode;
        operator Json::Value() const;
        static bool IsOk(const Json::Value &root);
        RecordMode &operator=(const Json::Value &root);
    };

    static Config *Instance();

    virtual ~Config();

    int Initialize(const std::string &path);

    void Close();

    int WriteToFile();

    bool IsResourceMode();

protected:
    explicit Config();

public:
    Video video_;
    Adv7842 adv7842_;
    Scene scene_;
    Display display_;
    RemoteLive remote_live_;
    LocalLive local_lives_;
    Record records_;
    SwitchCommand switch_cmd_;
    Web web_;
    Logger logger_;
    OsdTs osd_ts_;
    Osd osd_;
    RecordMode record_mode_;

private:
    std::string path_;
    bool init_;
};
} // namespace rs