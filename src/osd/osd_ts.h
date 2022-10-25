#pragma once

#include "global.h"

namespace rs
{

namespace osd_ts
{
struct Params
{
    int hdl;
    int vpss_grp;
    std::string font_file;
    int font_size;
    uint8_t font_color_r;
    uint8_t font_color_g;
    uint8_t font_color_b;
    int x;
    int y;
    std::string ts_format;
};
} // namespace osd_ts

class OsdTs
{
public:
    explicit OsdTs();

    virtual ~OsdTs();

    int Initialize(const osd_ts::Params &params);

    void Close();

private:
    osd_ts::Params params_;
    TTF_Font *font_;
    std::atomic<bool> run_;
    std::unique_ptr<std::thread> thread_;
    bool init_;
};
} // namespace rs