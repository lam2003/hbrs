#pragma once

#include "global.h"

namespace rs
{

class Config
{
public:
    struct Video
    {
        int frame_rate;

        struct Record
        {
            int width;
            int height;
            int bitrate;
        };

        struct Live
        {
            int width;
            int height;
            int bitrate;
        };

        Record record;
        Live live;
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
    };

    static Config *Instance();

    virtual ~Config();

    int Initialize(const std::string &path);

    void Close();

    int WriteToFile();

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