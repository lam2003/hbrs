#include "system/pciv_comm.h"
#include "common/utils.h"

namespace rs
{

using namespace pciv;

const int32_t PCIVComm::MsgPortBase = 100;
const int32_t PCIVComm::MaxPortNum = 3;
const int32_t PCIVComm::CommCMDPort = 0;
const int32_t PCIVComm::TransReadPort = 1;
const int32_t PCIVComm::TransWritePort = 2;

PCIVComm *PCIVComm::Instance()
{
    static PCIVComm *instance = new PCIVComm;
    return instance;
}

PCIVComm::PCIVComm() : init_(false)
{
}

PCIVComm::~PCIVComm()
{
    Close();
}

int32_t PCIVComm::Initialize()
{
    if (init_)
        return KInitialized;

    int32_t ret;

    ret = EnumChip(local_id_, remote_ids_);
    if (ret != KSuccess)
        return ret;

    remote_fds_.resize(PCIV_MAX_CHIPNUM);
    for (int32_t i = 0; i < PCIV_MAX_CHIPNUM; i++)
    {
        remote_fds_[i].resize(MaxPortNum);
        for (int32_t j = 0; j < MaxPortNum; j++)
            remote_fds_[i][j] = -1;
    }

    for (int32_t remote_id : remote_ids_)
    {
        ret = OpenPort(remote_id, CommCMDPort, remote_fds_);
        if (ret != KSuccess)
            return ret;
        ret = OpenPort(remote_id, TransReadPort, remote_fds_);
        if (ret != KSuccess)
            return ret;
        ret = OpenPort(remote_id, TransWritePort, remote_fds_);
        if (ret != KSuccess)
            return ret;
        ret = WaitConn(remote_id);
        if (ret != KSuccess)
            return ret;
    }

    init_ = true;

    return KSuccess;
}

void PCIVComm::Close()
{
    if (!init_)
        return;
    remote_fds_.resize(PCIV_MAX_CHIPNUM);
    for (int32_t i = 0; i < PCIV_MAX_CHIPNUM; i++)
    {
        remote_fds_[i].resize(MaxPortNum);
        for (int32_t j = 0; j < MaxPortNum; j++)
            if (remote_fds_[i][j] != -1)
                close(remote_fds_[i][j]);
    }

    init_ = false;
}

int32_t PCIVComm::WaitConn(int32_t remote_id)
{
    int32_t ret;

    const char *dev = "/dev/mcc_userdev";
    int32_t fd = open(dev, O_RDWR);
    if (fd <= 0)
    {
        log_e("open %s failed,%s", dev, strerror(errno));
        return KSystemError;
    }

    hi_mcc_handle_attr attr;
    attr.target_id = remote_id;
    attr.port = 1000;
    attr.priority = 0;

    ret = ioctl(fd, HI_MCC_IOC_CONNECT, &attr);
    if (ret != KSuccess)
    {
        log_e("ioctl HI_MCC_IOC_CONNECT failed,%s", strerror(errno));
        return KSystemError;
    }

    while (ioctl(fd, HI_MCC_IOC_CHECK, &attr))
        usleep(10000);

    log_d("chip[%d] connected", remote_id);

    close(fd);
    return KSuccess;
}

int32_t PCIVComm::OpenPort(int32_t remote_id, int32_t port, std::vector<std::vector<int32_t>> &remote_fds)
{
    int32_t ret;

    const char *dev = "/dev/mcc_userdev";
    int32_t fd = open(dev, O_RDWR | O_NONBLOCK);
    if (fd <= 0)
    {
        log_e("open %s failed,%s", dev, strerror(errno));
        return KSystemError;
    }

    hi_mcc_handle_attr attr;
    attr.target_id = remote_id;
    attr.port = port + MsgPortBase;
    attr.priority = 2;

    ret = ioctl(fd, HI_MCC_IOC_CONNECT, &attr);
    if (ret != KSuccess)
    {
        close(fd);
        log_e("ioctl HI_MCC_IOC_CONNECT failed,%s", strerror(errno));
        return KSystemError;
    }

    log_d("chip[%d] port[%d] fd[%d]", remote_id, port + MsgPortBase, fd);
    remote_fds[remote_id][port] = fd;
    return KSuccess;
}

int32_t PCIVComm::EnumChip(int32_t &local_id, std::vector<int32_t> &remote_ids)
{
    int32_t ret;

    const char *dev = "/dev/mcc_userdev";
    int32_t fd = open(dev, O_RDWR);
    if (fd < 0)
    {
        log_e("open %s failed,%s", dev, strerror(errno));
        return KSystemError;
    }

    hi_mcc_handle_attr attr;
    ret = ioctl(fd, HI_MCC_IOC_ATTR_INIT, &attr);
    if (ret < 0)
    {
        log_e("ioctl HI_MCC_IOC_ATTR_INIT failed,%s", strerror(errno));
        return KSystemError;
    }

    local_id = ioctl(fd, HI_MCC_IOC_GET_LOCAL_ID, &attr);
    if (local_id < 0)
    {
        log_e("ioctl HI_MCC_IOC_GET_LOCAL_ID failed,%s", strerror(errno));
        return KSystemError;
    }

    if (ioctl(fd, HI_MCC_IOC_GET_REMOTE_ID, &attr))
    {
        log_e("ioctl HI_MCC_IOC_GET_REMOTE_ID failed,%s", strerror(errno));
        return KSystemError;
    }

    remote_ids.clear();
    for (int32_t i = 0; i < HISI_MAX_MAP_DEV; i++)
    {
        if (attr.remote_id[i] != -1)
            remote_ids.push_back(attr.remote_id[i]);
    }

    attr.target_id = 1;
    attr.port = 0;
    attr.priority = 0;
    ret = ioctl(fd, HI_MCC_IOC_CONNECT, &attr);
    if (ret < 0)
    {
        log_e("ioctl HI_MCC_IOC_CONNECT failed,%s", strerror(errno));
        return KSystemError;
    }

    close(fd);
    return KSuccess;
}

int32_t PCIVComm::Send(int32_t remote_id, int32_t port, uint8_t *data, int32_t len)
{
    if (!init_)
        return KUnInitialized;

    int32_t ret;
    int32_t fd = remote_fds_[remote_id][port];

    int32_t rest_len = len;
    int32_t offset = 0;

    while (rest_len)
    {
    again:

        ret = write(fd, data + offset, rest_len);
        if (ret < 0 && errno == EINTR)
        {
            goto again;
        }
        else if (ret < 0)
        {
            log_e("write failed,%s", strerror(errno));
            return KSystemError;
        }
        offset += ret;
        rest_len -= ret;
    }
    return KSuccess;
}

int32_t PCIVComm::Recv(int32_t remote_id, int32_t port, uint8_t *data, int32_t len)
{
    if (!init_)
        return KUnInitialized;
    int32_t ret;
    int32_t fd = remote_fds_[remote_id][port];
    ret = read(fd, data, len);
    return ret;
}

const std::vector<int32_t> &PCIVComm::GetRemoteIds()
{
    return remote_ids_;
}

int32_t PCIVComm::GetTransReadPort()
{
    return TransReadPort;
}

int32_t PCIVComm::GetTransWritePort()
{
    return TransWritePort;
}

int32_t PCIVComm::GetCMDPort()
{
    return CommCMDPort;
}

} // namespace rs