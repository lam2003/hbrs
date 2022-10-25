#include "record/mp4_record.h"
#include "common/err_code.h"
#include "common/utils.h"
#include "common/bind_cpu.h"

namespace rs
{

using namespace mp4;

MP4Record::MP4Record() : run_(false),
                         thread_(nullptr),
                         init_(false)
{
}

MP4Record::~MP4Record()
{
    Close();
}

int MP4Record::Initialize(const Params &params)
{
    if (init_)
        return KInitialized;

    log_d("MP4REC start,filename:%s,width:%d,height:%d,frame_rate:%d,samplate_rate:%d", params.filename.c_str(),
          params.width,
          params.height,
          params.frame_rate,
          params.samplate_rate);
    params_ = params;

    run_ = true;
    thread_ = std::unique_ptr<std::thread>(new std::thread([this]() {
        CPUBind::SetCPU(1);
        std::string filename;
        MP4Muxer muxer;
        Frame frame;
        MMZBuffer mmz_buffer(2 * 1024 * 1024);
        bool wait_sps_pps;
        uint64_t cur_frame_num;

        int segment_no = 0;
        uint64_t need_frame_num = params_.segment_duration * params_.frame_rate;
        bool init = false;

        while (run_)
        {
            int ret;

            if (!init)
            {
                if (params_.need_to_segment)
                {
                    filename = Utils::ModifyMP4FileName(params_.filename, segment_no);
                    segment_no++;
                }
                else
                {
                    filename = params_.filename;
                }

                ret = muxer.Initialize(params_.width, params_.height, params_.frame_rate, params_.samplate_rate, filename);
                if (ret != KSuccess)
                    return;

                mux_.lock();
                buffer_.Clear();
                mux_.unlock();

                wait_sps_pps = true;
                cur_frame_num = 0;
                init = true;
            }

            {
                std::unique_lock<std::mutex> lock(mux_);
                if (buffer_.Get(reinterpret_cast<uint8_t *>(&frame), sizeof(frame)))
                {

                    if (frame.type == Frame::AUDIO)
                    {
                        memcpy(mmz_buffer.vir_addr, buffer_.GetCurrentPos(), frame.data.aframe.len);
                        frame.data.aframe.data = mmz_buffer.vir_addr;
                        buffer_.Consume(frame.data.aframe.len);
                    }
                    else
                    {
                        memcpy(mmz_buffer.vir_addr, buffer_.GetCurrentPos(), frame.data.vframe.len);
                        frame.data.vframe.data = mmz_buffer.vir_addr;
                        buffer_.Consume(frame.data.vframe.len);
                    }
                }
                else if (run_)
                {
                    cond_.wait(lock);
                    continue;
                }
                else
                {
                    continue;
                }
            }

            if (frame.type == Frame::VIDEO && frame.data.vframe.type == H264E_NALU_SPS)
                wait_sps_pps = false;

            if (!wait_sps_pps)
            {
                if (frame.type == Frame::AUDIO)
                {
                    ret = muxer.WriteAudioFrame(frame.data.aframe);
                    if (ret != KSuccess)
                    {
                        muxer.Close();
                        return;
                    }
                }
                else
                {
                    if (frame.data.vframe.type != H264E_NALU_SPS && frame.data.vframe.type != H264E_NALU_PPS && frame.data.vframe.type != H264E_NALU_SEI)
                        cur_frame_num++;
                    ret = muxer.WriteVideoFrame(frame.data.vframe);
                    if (ret != KSuccess)
                    {
                        muxer.Close();
                        return;
                    }
                }
            }

            if (params_.need_to_segment && cur_frame_num >= need_frame_num)
            {
                muxer.Close();
                init = false;
            }
        }

        muxer.Close();
    }));

    init_ = true;

    return KSuccess;
}

void MP4Record::Close()
{
    if (!init_)
        return;

    log_d("MP4REC stop,filename:%s", params_.filename.c_str());
    run_ = false;
    cond_.notify_all();
    thread_->join();
    thread_.reset();
    thread_ = nullptr;

    init_ = false;
}

void MP4Record::OnFrame(const VENCFrame &video_frame)
{
    if (!init_)
        return;

    std::unique_lock<std::mutex> lock(mux_);
    if (buffer_.FreeSpace() < sizeof(Frame) + video_frame.len)
        return;

    Frame frame;
    frame.type = Frame::VIDEO;
    frame.data.vframe = video_frame;

    buffer_.Append(reinterpret_cast<uint8_t *>(&frame), sizeof(frame));
    buffer_.Append(video_frame.data, video_frame.len);
    cond_.notify_one();
}

void MP4Record::OnFrame(const AENCFrame &audio_frame)
{
    if (!init_)
        return;

    std::unique_lock<std::mutex> lock(mux_);
    if (buffer_.FreeSpace() < sizeof(Frame) + audio_frame.len)
        return;

    Frame frame;
    frame.type = Frame::AUDIO;
    frame.data.aframe = audio_frame;

    buffer_.Append(reinterpret_cast<uint8_t *>(&frame), sizeof(frame));
    buffer_.Append(audio_frame.data, audio_frame.len);
    cond_.notify_one();
}

Params::operator Json::Value() const
{
    Json::Value root;
    root["width"] = width;
    root["height"] = height;
    root["frame_rate"] = frame_rate;
    root["samplate_rate"] = samplate_rate;
    root["filename"] = filename;
    root["segment_duration"] = segment_duration;
    root["need_to_segment"] = need_to_segment;
    return root;
}

bool Params::IsOk(const Json::Value &root)
{
    if (!root.isObject() ||
        !root.isMember("filename") ||
        !root["filename"].isString() ||
        !root.isMember("segment_duration") ||
        !root["segment_duration"].isInt() ||
        !root.isMember("need_to_segment") ||
        !root["need_to_segment"].isBool())
        return false;
    return true;
}

Params &Params::operator=(const Json::Value &root)
{
    filename = root["filename"].asString();
    segment_duration = root["segment_duration"].asInt();
    need_to_segment = root["need_to_segment"].asBool();
    return *this;
}

} // namespace rs