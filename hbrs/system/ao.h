#pragma once

namespace rs
{

namespace ao
{
struct Params
{
    int dev;
    int chn;
};
} // namespace ao
class AudioOutput
{
public:
    explicit AudioOutput();

    virtual ~AudioOutput();

    int Initialize(const ao::Params &params);

    void Close();

private:
    ao::Params params_;
    bool init_;
};
} // namespace rs