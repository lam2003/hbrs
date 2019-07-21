#pragma once

#include "global.h"

namespace rs
{

class Config
{
public:
    struct Video
    {
        int record_width;
        int record_height;
        int record_bitrate;

        int live_width;
        int live_height;
        int live_bitrate;
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
    };

    struct System
    {
        struct Chn
        {
            int chn;
            RECT_S rect;
        };

        VO_INTF_SYNC_E disp_vo_intf_sync;
        std::vector<Chn> chns;
        std::map<int, RS_SCENE> mapping;
        ADV7842_MODE pc_capture_mode;
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
    Scene scene_;
    System system_;

private:
    std::string path_;
    bool init_;
};
} // namespace rs