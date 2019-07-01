
//self
#include "system/sig_detect.h"
#include "common/buffer.h"
#include "common/err_code.h"
#include "common/utils.h"

namespace rs
{

static int Recv(pciv::Context *ctx, int remote_id, int port, uint8_t *tmp_buf, int32_t buf_len, Buffer<allocator_1k> &msg_buf, pciv::Msg &msg, int try_time)
{
    int ret;
    while (try_time-- && msg_buf.Size() < sizeof(msg))
    {
        ret = ctx->Recv(remote_id, port, tmp_buf, buf_len, 500000); //500ms
        if (ret > 0)
        {
            if (!msg_buf.Append(tmp_buf, ret))
            {
                log_e("append data to msg buf failed");
                return KNotEnoughBuf;
            }
        }
        else if (ret < 0)
            return ret;
    }

    if (msg_buf.Size() >= sizeof(msg))
    {
        msg_buf.Get(reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
        msg_buf.Consume(sizeof(msg));
        return KSuccess;
    }

    log_e("recv timeout");
    return KTimeout;
}

SigDetect::~SigDetect()
{
    Close();
}

SigDetect::SigDetect() : ctx_(nullptr),
                         run_(false),
                         thread_(nullptr),
                         init_(false)
{
}

SigDetect *SigDetect::Instance()
{
    static SigDetect *instance = new SigDetect;
    return instance;
}

int SigDetect::Initialize(pciv::Context *ctx)
{
    if (init_)
        return KInitialized;

    ctx_ = ctx;
    fmts_.resize(RS_TOTAL_SCENE_NUM);

    run_ = true;
    thread_ = std::unique_ptr<std::thread>(new std::thread([this]() {
        int ret;

        const char *tw6874_1_dev = "/dev/tw6874_driver_1";
        int tw6874_1_fd = open(tw6874_1_dev, O_RDWR);
        if (tw6874_1_fd < 0)
        {
            log_e("open %s failed,%s", tw6874_1_dev, strerror(errno));
            return;
        }

        const char *tw6874_2_dev = "/dev/tw6874_driver_2";
        int tw6874_2_fd = open(tw6874_2_dev, O_RDWR);
        if (tw6874_2_fd < 0)
        {
            log_e("open %s failed,%s", tw6874_2_dev, strerror(errno));
            return;
        }

        std::vector<VideoInputFormat> tmp_fmts;
        tmp_fmts.resize(RS_TOTAL_SCENE_NUM);
        for (size_t i = 0; i < tmp_fmts.size(); i++)
            memset(&tmp_fmts[i], 0, sizeof(tmp_fmts[i]));

        pciv::Msg msg;
        uint8_t tmp_buf[1024];
        Buffer<allocator_1k> msg_buf;

        while (run_)
        {
            {
                int cur_chn;
                for (int i = RS_SLAVE_SDI_BASE; i < RS_SLAVE_SDI_NUM; i++)
                {
                    cur_chn = i;
                    ret = ioctl(tw6874_1_fd, CMD_CHECK_ADN_LOCK_SDI, &cur_chn);
                    if (ret != KSuccess)
                    {
                        log_e("ioctl CMD_CHECK_ADN_LOCK_SDI failed,%s", strerror(errno));
                        return;
                    }
                    tmp_fmts[Tw6874_1_DevChn2Scene[i]] = Utils::GetVIFmt(static_cast<hd_dis_resolution_e>(cur_chn));
                }
                for (int i = RS_MASTER_SDI_BASE; i < RS_MASTER_SDI_NUM; i++)
                {
                    cur_chn = i;
                    ret = ioctl(tw6874_2_fd, CMD_CHECK_ADN_LOCK_SDI, &cur_chn);
                    if (ret != KSuccess)
                    {
                        log_e("ioctl CMD_CHECK_ADN_LOCK_SDI failed,%s", strerror(errno));
                        return;
                    }
                    tmp_fmts[Tw6874_2_DevChn2Scene[i]] = Utils::GetVIFmt(static_cast<hd_dis_resolution_e>(cur_chn));
                }

                msg.type = pciv::Msg::Type::QUERY_ADV7842;
                ret = ctx_->Send(RS_PCIV_SLAVE1_ID, RS_PCIV_CMD_PORT, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
                if (ret != KSuccess)
                    return;

                ret = Recv(ctx_, RS_PCIV_SLAVE1_ID, RS_PCIV_CMD_PORT, tmp_buf, sizeof(tmp_buf), msg_buf, msg, 3);
                if (ret != KSuccess)
                    return;
                if (msg.type != pciv::Msg::Type::ACK)
                {
                    log_e("unknow msg type:%d", msg.type);
                    return;
                }

                memcpy(&tmp_fmts[PC_CAPTURE], msg.data, sizeof(tmp_fmts[PC_CAPTURE]));

                pciv::Tw6874Query query;
                for (int i = TEA_FULL_VIEW; i <= BLACK_BOARD_FEATURE; i++)
                    query.fmts[i - TEA_FULL_VIEW] = tmp_fmts[i];

                msg.type = pciv::Msg::Type::QUERY_TW6874;
                memcpy(msg.data, &query, sizeof(query));
                ret = ctx_->Send(RS_PCIV_SLAVE3_ID, RS_PCIV_CMD_PORT, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
                if (ret != KSuccess)
                    return;

                {
                    std::unique_lock<std::mutex> lock(mux_);
                    std::swap(tmp_fmts, fmts_);
                }

                usleep(1000000);
            }
        }
    }));

    init_ = true;

    return KSuccess;
}

void SigDetect::Close()
{
    if (!init_)
        return;

    run_ = false;
    thread_->join();
    thread_.reset();
    thread_ = nullptr;

    fmts_.clear();

    ctx_ = nullptr;

    init_ = false;
}

} // namespace rs