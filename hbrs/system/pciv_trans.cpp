#include "system/pciv_trans.h"

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

    run_ = false;
    for (size_t i = 0; i < threads_.size(); i++)
    {
        threads_[i]->join();
        threads_[i].reset();
        threads_[i] = nullptr;
    }
    threads_.clear();

    init_ = false;
}

int PCIVTrans::Initialize(pciv::Context *ctx)
{
    if (init_)
        return KInitialized;

    ctx_ = ctx;

    run_ = true;
    const std::vector<int> &remote_ids = ctx_->GetRemoteIds();
    for (int remote_id : remote_ids)
    {
        static std::mutex g_Mux;
        std::shared_ptr<std::thread> thr = std::make_shared<std::thread>([this, remote_id]() {
            int ret;

            uint32_t phy_addr[1];
            uint8_t *vir_addr;
            uint8_t tmp_buf[1024];

            {
                std::unique_lock<std::mutex> lock(g_Mux);
                ret = HI_MPI_PCIV_Malloc(PCIV_WINDOW_SIZE, 1, phy_addr);
                if (ret != KSuccess)
                {
                    log_e("HI_MPI_PCIV_Malloc failed with %#x", ret);
                    return;
                }

                vir_addr = static_cast<uint8_t *>(HI_MPI_SYS_Mmap(phy_addr[0], PCIV_WINDOW_SIZE));
                if (vir_addr == nullptr)
                {
                    HI_MPI_PCIV_Free(1, phy_addr);
                    log_e("HI_MPI_SYS_Mmap failed");
                    return;
                }
            }

            Msg msg;
            Buffer<allocator_1k> msg_buf;
            while (run_)
            {
                do
                {
                    ret = ctx_->Recv(remote_id, ctx_->GetTransReadPort(), tmp_buf, sizeof(tmp_buf));
                    if (ret > 0)
                    {
                        if (!msg_buf.Append(tmp_buf, ret))
                        {
                            HI_MPI_SYS_Munmap(vir_addr, PCIV_WINDOW_SIZE);
                            HI_MPI_PCIV_Free(1, phy_addr);
                            log_e("buffer fill");
                            return;
                        }
                    }
                    else
                    {
                        usleep(10000); //10ms
                    }
                } while (run_ && msg_buf.Size() < sizeof(msg));

                if (msg_buf.Size() >= sizeof(msg))
                {

                    msg_buf.Get(reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
                    msg_buf.Consume(sizeof(msg));

                    if (msg.type != Msg::Type::WRITE_DONE)
                        continue;

                    PosInfo *pos_info = reinterpret_cast<PosInfo *>(msg.data);

                    uint8_t *data = vir_addr + pos_info->start_pos;
                    uint32_t len = pos_info->end_pos - pos_info->start_pos;

                    VDEC_STREAM_S st;
                    for (uint8_t *cur_pos = data; cur_pos < data + len;)
                    {
                        StreamInfo *stream_info = reinterpret_cast<StreamInfo *>(cur_pos);
                        st.pu8Addr = cur_pos + sizeof(StreamInfo);
                        st.u32Len = stream_info->len;
                        st.u64PTS = stream_info->pts;
                        HI_MPI_VDEC_SendStream(stream_info->vdec_chn, &st, HI_IO_NOBLOCK);

                        cur_pos += (sizeof(StreamInfo) + stream_info->align_len);
                    }

                    msg.type = Msg::Type::READ_DONE;
                    pos_info->start_pos = pos_info->end_pos;
                    ret = ctx_->Send(remote_id, ctx_->GetTransWritePort(), reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
                    if (ret != KSuccess)
                    {
                        HI_MPI_SYS_Munmap(vir_addr, PCIV_WINDOW_SIZE);
                        HI_MPI_PCIV_Free(1, phy_addr);
                        return;
                    }
                }
            }
            HI_MPI_SYS_Munmap(vir_addr, PCIV_WINDOW_SIZE);
            HI_MPI_PCIV_Free(1, phy_addr);
        });

        threads_.push_back(thr);

        init_ = true;
    }
    return KSuccess;
}
} // namespace rs