//stl
#include <map>
//drive
#include <tw6874_ioctl_cmd.h>
//self
#include "system/sig_detect.h"
#include "common/utils.h"

namespace rs
{

static std::map<int, RS_SCENE> Tw6874_1_DevChn2Scene = {
    {3, TEA_FULL_VIEW},
    {2, STU_FULL_VIEW},
    {1, BLACK_BOARD_FEATURE}};

static std::map<int, RS_SCENE> Tw6874_2_DevChn2Scene = {
    {1, TEA_FEATURE},
    {0, STU_FEATURE}};

SigDetect::~SigDetect()
{
    Close();
}

SigDetect::SigDetect() : init_(false)
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

                ret = Utils::Recv(ctx_, RS_PCIV_SLAVE1_ID, RS_PCIV_CMD_PORT, tmp_buf, sizeof(tmp_buf), msg_buf, msg, 3);
                if (ret != KSuccess)
                    return;
                if (msg.type != pciv::Msg::Type::ACK)
                {
                    log_e("unknow msg type:%d", msg.type);
                    return;
                }

                memcpy(&tmp_fmts[PC_CAPTURE], msg.data, sizeof(tmp_fmts[PC_CAPTURE]));
                for (size_t i = 0; i < tmp_fmts.size(); i++)
                {
                    VideoInputFormat fmt = tmp_fmts[i];
                    // log_d("chn:%d,has_signal:%d,fmt.width:%d,height:%d,fps:%d,interlaced:%d",
                    //       i,
                    //       fmt.has_signal,
                    //       fmt.width,
                    //       fmt.height,
                    //       fmt.frame_rate,
                    //       fmt.interlaced);
                }
                sleep(1);
            }
        }
    }));

    return KSuccess;
}

void SigDetect::Close() {}

} // namespace rs