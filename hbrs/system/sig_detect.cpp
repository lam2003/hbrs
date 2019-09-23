
//self
#include "system/sig_detect.h"
#include "common/buffer.h"
#include "common/err_code.h"
#include "common/utils.h"

namespace rs
{

static int Recv(std::shared_ptr<PCIVComm> pciv_comm, int remote_id, int port, uint8_t *tmp_buf, int32_t buf_len, Buffer<allocator_1k> &msg_buf, pciv::Msg &msg, int try_time)
{
    int ret;
    while (try_time-- && msg_buf.Size() < sizeof(msg))
    {
        ret = pciv_comm->Recv(remote_id, port, tmp_buf, buf_len, 500000); //500ms
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

SigDetect::SigDetect() : status_listeners_(nullptr),
                         pciv_comm_(nullptr),
                         run_(false),
                         thread_(nullptr),
                         init_(false)
{
}

void PrintSignal(const std::vector<VideoInputFormat> &fmts)
{
    log_d("############signal changed############");

    for (size_t i = 0; i < fmts.size(); i++)
    {
        if (fmts[i].has_signal)
        {
            log_d("[%d]width:%d,height:%d,interlaced:%d,frame_rate:%d", i, fmts[i].width, fmts[i].height, fmts[i].interlaced, fmts[i].frame_rate);
        }
        else
        {
            log_d("[%d]no signal", i);
        }
    }
    log_d("######################################");
}

int SigDetect::Initialize(std::shared_ptr<PCIVComm> pciv_comm, ADV7842_MODE mode)
{
    if (init_)
        return KInitialized;

    log_d("SIG_DETECT start");

    pciv_comm_ = pciv_comm;
    fmts_.resize(RS_TOTAL_SCENE_NUM);

    run_ = true;
    thread_ = std::unique_ptr<std::thread>(new std::thread([this, mode]() {
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

        std::vector<int> no_signal_times;
        no_signal_times.resize(RS_TOTAL_SCENE_NUM);
        for (size_t i = 0; i < no_signal_times.size(); i++)
            no_signal_times[i] = 0;

        std::vector<bool> changes;
        changes.resize(RS_TOTAL_SCENE_NUM);
        for (size_t i = 0; i < changes.size(); i++)
            changes[i] = false;

        pciv::Msg msg;
        uint8_t tmp_buf[1024];
        Buffer<allocator_1k> msg_buf;

        msg.type = pciv::Msg::Type::CONF_ADV7842;
        pciv::Adv7842Conf *conf = reinterpret_cast<pciv::Adv7842Conf *>(msg.data);
        conf->mode = mode;
        ret = pciv_comm_->Send(RS_PCIV_SLAVE1_ID, RS_PCIV_CMD_PORT, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
        if (ret != KSuccess)
            return;

        while (run_)
        {

            int cur_chn;
            for (int i = RS_SLAVE_SDI_BASE; i < RS_SLAVE_SDI_NUM && run_; i++)
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
            for (int i = RS_MASTER_SDI_BASE; i < RS_MASTER_SDI_NUM && run_; i++)
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
            if (run_)
            {
                msg.type = pciv::Msg::Type::QUERY_ADV7842;
                {
                    std::unique_lock<std::mutex> lock(mux_);
                    ret = pciv_comm_->Send(RS_PCIV_SLAVE1_ID, RS_PCIV_CMD_PORT, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
                    if (ret != KSuccess)
                        return;

                    ret = Recv(pciv_comm_, RS_PCIV_SLAVE1_ID, RS_PCIV_CMD_PORT, tmp_buf, sizeof(tmp_buf), msg_buf, msg, 3);
                    if (ret != KSuccess)
                    {
                        memset(&msg, 0, sizeof(msg));
                        msg.type = pciv::Msg::Type::ACK;
                    }
                }
                if (msg.type != pciv::Msg::Type::ACK)
                {
                    log_e("unknow msg type:%d", msg.type);
                    return;
                }

                memcpy(&tmp_fmts[PC_CAPTURE], msg.data, sizeof(tmp_fmts[PC_CAPTURE]));

                bool changed = false;
                for (int i = TEA_FEA; i <= PC_CAPTURE; i++)
                {
                    if (fmts_[i] != tmp_fmts[i])
                    {
                        changed = true;
                        changes[i] = true;
                        fmts_[i] = tmp_fmts[i];
                    }
                    else
                    {
                        changes[i] = false;
                    }
                }

                if (changes[TEA_FULL] || changes[STU_FULL] || changes[BB_FEA])
                {
                    pciv::Tw6874Query query;
                    for (int i = TEA_FULL; i <= BB_FEA; i++)
                        query.fmts[i - TEA_FULL] = fmts_[i];

                    msg.type = pciv::Msg::Type::QUERY_TW6874;
                    memcpy(msg.data, &query, sizeof(query));
                    {
                        std::unique_lock<std::mutex> lock(mux_);
                        ret = pciv_comm_->Send(RS_PCIV_SLAVE3_ID, RS_PCIV_CMD_PORT, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
                        if (ret != KSuccess)
                            return;
                    }
                }
                {
                    std::unique_lock<std::mutex> lock(mux_);
                    if (changes[TEA_FEA])
                    {
                        for (size_t i = 0; i < listeners_.size(); i++)
                            listeners_[i]->OnChange(fmts_[TEA_FEA], Scene2ViChn[TEA_FEA]);
                    }

                    if (changes[STU_FEA])
                    {
                        for (size_t i = 0; i < listeners_.size(); i++)
                            listeners_[i]->OnChange(fmts_[STU_FEA], Scene2ViChn[STU_FEA]);
                    }
                }
                if (changed)
                    PrintSignal(fmts_);

                {
                    std::unique_lock<std::mutex> lock(mux_);
                    if (status_listeners_ != nullptr)
                        status_listeners_->OnUpdate(fmts_);
                }

                int wait_time = 20;
                while (run_ && wait_time--) //休眠10秒
                    usleep(500000);         //500ms,快速退出
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

    log_d("SIG_DETECT stop");

    run_ = false;
    thread_->join();
    thread_.reset();
    thread_ = nullptr;

    fmts_.clear();
    listeners_.clear();

    pciv_comm_.reset();
    pciv_comm_ = nullptr;

    init_ = false;
}

void SigDetect::AddVIFmtListener(std::shared_ptr<VIFmtListener> listener)
{
    std::unique_lock<std::mutex> lock(mux_);
    listeners_.push_back(listener);
}

void SigDetect::RemoveAllVIFmtListener()
{
    std::unique_lock<std::mutex> lock(mux_);
    listeners_.clear();
}

void SigDetect::SetSignalStatusListener(std::shared_ptr<SignalStatusListener> listener)
{
    std::unique_lock<std::mutex> lock(mux_);
    status_listeners_ = listener;
}

int SigDetect::SetPCCaptureMode(ADV7842_MODE mode)
{
    if (!init_)
        return KUnInitialized;
    pciv::Msg msg;
    msg.type = pciv::Msg::Type::CONF_ADV7842;
    pciv::Adv7842Conf *conf = reinterpret_cast<pciv::Adv7842Conf *>(msg.data);
    conf->mode = mode;
    int ret = pciv_comm_->Send(RS_PCIV_SLAVE1_ID, RS_PCIV_CMD_PORT, reinterpret_cast<uint8_t *>(&msg), sizeof(msg));
    if (ret != KSuccess)
        return ret;
    return KSuccess;
}

} // namespace rs