#include "system/pciv_trans.h"
#include "common/buffer.h"

namespace rs
{
using namespace pciv;

PCIVTrans::PCIVTrans() : run_(false),
                         ctx_(nullptr),
                         init_(false)
{
}

PCIVTrans::~PCIVTrans()
{
    Close();
}

PCIVTrans *PCIVTrans::Instance()
{
    static PCIVTrans *instance = new PCIVTrans;
    return instance;
}

void PCIVTrans::Close()
{
    if (!init_)
        return;

    int ret;

    run_ = false;
    for (size_t i = 0; i < threads_.size(); i++)
    {
        threads_[i]->join();
        threads_[i].reset();
        threads_[i] = nullptr;
    }

    for (size_t i = 0; i < buffers_.size(); i++)
    {
        ret = HI_MPI_SYS_Munmap(buffers_[i].vir_addr, PCIV_WINDOW_SIZE);
        if (ret != KSuccess)
            log_e("HI_MPI_SYS_Munmap failed with %#x", ret);
        ret = HI_MPI_PCIV_Free(1, buffers_[i].phy_addr);
        if (ret != KSuccess)
            log_e("HI_MPI_PCIV_Free failed with %#x", ret);
    }

    buffers_.clear();
    threads_.clear();
    video_sinks_.clear();

    init_ = false;
}

void PCIVTrans::UnpackAndSendStream(uint8_t *data, int32_t len, const std::vector<VideoSink<VDEC_STREAM_S> *> &video_sinks)
{
    VDEC_STREAM_S st;
    for (uint8_t *cur_pos = data; cur_pos < data + len;)
    {
        StreamInfo *stream_info = reinterpret_cast<StreamInfo *>(cur_pos);
        st.pu8Addr = cur_pos + sizeof(StreamInfo);
        st.u32Len = stream_info->len;
        st.u64PTS = stream_info->pts;
        for (VideoSink<VDEC_STREAM_S> *sink : video_sinks)
            sink->OnFrame(st, stream_info->vdec_chn);
        cur_pos += (sizeof(StreamInfo) + stream_info->align_len);
    }
}

static bool Recv(pciv::Context *ctx, int remote_id, uint8_t *tmp_buf, int32_t buf_len, rs::Buffer<allocator_1k> &msg_buf, const std::atomic<bool> &run, Msg &msg)
{
    int ret;
    do
    {
        ret = ctx->Recv(remote_id, ctx->GetTransReadPort(), tmp_buf, buf_len);
        if (ret > 0)
        {
            if (!msg_buf.Append(tmp_buf, ret))
            {
                log_e("append data to msg_buf failed");
                log_e("error:%s", make_error_code(static_cast<err_code>(KNotEnoughBuf)).message().c_str());
                return false;
            }
        }
        else
        {
            usleep(10000); //10ms
        }

    } while (run && msg_buf.Size() < sizeof(msg));

    if (msg_buf.Size() >= sizeof(msg))
    {
        msg_buf.Get(reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
        msg_buf.Consume(sizeof(msg));
        return true;
    }

    return false;
}

int32_t PCIVTrans::Initialize(pciv::Context *ctx)
{
    if (init_)
        return KInitialized;

    int ret;

    ctx_ = ctx;

    run_ = true;
    const std::vector<int32_t> &remote_ids = ctx_->GetRemoteIds();
    for (int32_t remote_id : remote_ids)
    {
        pciv::Buffer buf;
        ret = HI_MPI_PCIV_Malloc(PCIV_WINDOW_SIZE, 1, buf.phy_addr);
        if (ret != KSuccess)
        {
            log_e("HI_MPI_PCIV_Malloc failed with %#x", ret);
            return KSDKError;
        }

        buf.vir_addr = static_cast<uint8_t *>(HI_MPI_SYS_Mmap(buf.phy_addr[0], PCIV_WINDOW_SIZE));
        if (buf.vir_addr == nullptr)
        {
            log_e("HI_MPI_SYS_Mmap failed");
            return KSDKError;
        }

        {
            //发送pciv主片内存信息
            Msg msg;
            msg.type = Msg::Type::NOTIFY_MEMORY;
            MemoryInfo *mem_info = reinterpret_cast<MemoryInfo *>(msg.data);
            mem_info->phy_addr = buf.phy_addr[0];

            ret = ctx_->Send(remote_id, ctx_->GetCMDPort(), reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
            if (ret != KSuccess)
                return ret;
        }

        buffers_.push_back(buf);

        std::shared_ptr<std::thread> thr = std::make_shared<std::thread>([this, buf, remote_id]() {
            int32_t ret;

            Msg msg;
            uint8_t tmp_buf[1024];
            rs::Buffer<allocator_1k> msg_buf;
            uint8_t *vir_addr = buf.vir_addr;

            while (run_)
            {
                if (Recv(ctx_, remote_id, tmp_buf, sizeof(tmp_buf), msg_buf, run_, msg))
                {
                    if (msg.type != Msg::Type::WRITE_DONE)
                    {
                        log_e("unknow msg type:%d", msg.type);
                        continue;
                    }

                    PosInfo *tmp = reinterpret_cast<PosInfo *>(msg.data);
                    printf("start_pos:%d end_pos:%d\n", tmp->start_pos, tmp->end_pos);
                    int32_t len = tmp->end_pos - tmp->start_pos;
                    uint8_t *data = vir_addr + tmp->start_pos;

                    video_sinks_mux_.lock();
                    UnpackAndSendStream(data, len, video_sinks_);
                    video_sinks_mux_.unlock();

                    msg.type = Msg::Type::READ_DONE;

                    ret = ctx_->Send(remote_id, ctx_->GetTransWritePort(), reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
                    if (ret != KSuccess)
                    {
                        log_e("error:%s", make_error_code(static_cast<err_code>(ret)).message().c_str());
                        return;
                    }
                }
            }
        });

        threads_.push_back(thr);
    }
    init_ = true;
    return KSuccess;
}

void PCIVTrans::AddVideoSink(VideoSink<VDEC_STREAM_S> *video_sink)
{
    std::unique_lock<std::mutex> lock(video_sinks_mux_);
    video_sinks_.push_back(video_sink);
}

void PCIVTrans::RemoveAllVideoSink()
{
    std::unique_lock<std::mutex> lock(video_sinks_mux_);
    video_sinks_.clear();
}
} // namespace rs