#pragma once

#include "global.h"

namespace rs
{

namespace osd
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
    std::string content;
};
} // namespace osd

class Osd
{
public:
    explicit Osd();

    virtual ~Osd();

    int Initialize(const osd::Params &params);

    void Close();

private:
    osd::Params params_;
    TTF_Font *font_;
    bool init_;
};
} // namespace rs