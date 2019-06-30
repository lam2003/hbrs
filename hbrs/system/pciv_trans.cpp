#include "system/pciv_trans.h"
#include "common/utils.h"

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

    for (size_t i = 0; i < bufs_.size(); i++)
    {
        ret = HI_MPI_SYS_Munmap(bufs_[i].vir_addr, RS_PCIV_WINDOW_SIZE);
        if (ret != KSuccess)
            log_e("HI_MPI_SYS_Munmap failed with %#x", ret);
        ret = HI_MPI_PCIV_Free(1, bufs_[i].phy_addr);
        if (ret != KSuccess)
            log_e("HI_MPI_PCIV_Free failed with %#x", ret);
    }

    bufs_.clear();
    threads_.clear();
    sinks_.clear();

    init_ = false;
}

void PCIVTrans::UnpackAndSendStream(uint8_t *data, int32_t len, const std::vector<VideoSink<VDEC_STREAM_S> *> &sinks)
{
    VDEC_STREAM_S st;
    for (uint8_t *cur_pos = data; cur_pos < data + len;)
    {
        StreamInfo *stream_info = reinterpret_cast<StreamInfo *>(cur_pos);
        st.pu8Addr = cur_pos + sizeof(StreamInfo);
        st.u32Len = stream_info->len;
        st.u64PTS = stream_info->pts;
        for (VideoSink<VDEC_STREAM_S> *sink : sinks)
            sink->OnFrame(st, stream_info->vdec_chn);
        cur_pos += (sizeof(StreamInfo) + stream_info->align_len);
    }
}

int32_t PCIVTrans::Initialize(pciv::Context *ctx)
{
    if (init_)
        return KInitialized;

    int ret;

    ctx_ = ctx;
    std::vector<int> remote_ids = {RS_PCIV_SLAVE1_ID, RS_PCIV_SLAVE3_ID};

    run_ = true;
    for (int32_t remote_id : remote_ids)
    {
        PCIVBuffer buf;
        ret = HI_MPI_PCIV_Malloc(RS_PCIV_WINDOW_SIZE, 1, buf.phy_addr);
        if (ret != KSuccess)
        {
            log_e("HI_MPI_PCIV_Malloc failed with %#x", ret);
            return KSDKError;
        }

        buf.vir_addr = reinterpret_cast<uint8_t *>(HI_MPI_SYS_Mmap(buf.phy_addr[0], RS_PCIV_WINDOW_SIZE));
        if (buf.vir_addr == nullptr)
        {
            log_e("HI_MPI_SYS_Mmap failed");
            return KSDKError;
        }

        //发送pciv主片内存信息
        Msg msg;
        msg.type = Msg::Type::START_TRANS;
        MemoryInfo *mem_info = reinterpret_cast<MemoryInfo *>(msg.data);
        mem_info->phy_addr = buf.phy_addr[0];
        bufs_.push_back(buf);

        ret = ctx_->Send(remote_id, RS_PCIV_CMD_PORT, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
        if (ret != KSuccess)
            return ret;

        std::shared_ptr<std::thread> thr = std::make_shared<std::thread>([this, buf, remote_id]() {
            int32_t ret;
            Msg msg;
            uint8_t tmp_buf[1024];
            Buffer<allocator_1k> msg_buf;

            while (run_)
            {
                ret = Utils::Recv(ctx_, remote_id, RS_PCIV_TRANS_READ_PORT, tmp_buf, sizeof(tmp_buf), msg_buf, run_, msg);
                if (ret != KSuccess)
                    return;

                if (!run_)
                    break;

                if (msg.type == Msg::Type::WRITE_DONE)
                {
                    PosInfo *pos_info = reinterpret_cast<PosInfo *>(msg.data);
                    // printf("start_pos:%d end_pos:%d\n", pos_info->start_pos, pos_info->end_pos);
                    int32_t len = pos_info->end_pos - pos_info->start_pos;
                    uint8_t *data = buf.vir_addr + pos_info->start_pos;

                    mux_.lock();
                    UnpackAndSendStream(data, len, sinks_);
                    mux_.unlock();

                    msg.type = Msg::Type::READ_DONE;
                    ret = ctx_->Send(remote_id, RS_PCIV_TRANS_WRITE_PORT, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
                    if (ret != KSuccess)
                        return;
                }
                else
                {
                    log_e("unknow msg type:%d", msg.type);
                    continue;
                }
            }
        });

        threads_.push_back(thr);
    }

    init_ = true;

    return KSuccess;
}

void PCIVTrans::AddVideoSink(VideoSink<VDEC_STREAM_S> *sink)
{
    std::unique_lock<std::mutex> lock(mux_);
    sinks_.push_back(sink);
}

void PCIVTrans::RemoveAllVideoSink()
{
    std::unique_lock<std::mutex> lock(mux_);
    sinks_.clear();
}
} // namespace rs