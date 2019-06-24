#include "system/pciv_comm.h"
#include "common/utils.h"

namespace rs
{

using namespace pciv;

const int PCIVComm::MsgPortBase = 100;
const int PCIVComm::MaxPortNum = 3;
const int PCIVComm::CommCMDPort = 0;
const int PCIVComm::TransReadPort = 1;
const int PCIVComm::TransWritePort = 2;

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

int PCIVComm::Initialize()
{
    if (init_)
        return KInitialized;

    int ret;

    ret = EnumChip(local_id_, remote_ids_);
    if (ret != KSuccess)
        return ret;

    remote_fds_.resize(PCIV_MAX_CHIPNUM);
    for (int i = 0; i < PCIV_MAX_CHIPNUM; i++)
    {
        remote_fds_[i].resize(MaxPortNum);
        for (int j = 0; j < MaxPortNum; j++)
            remote_fds_[i][j] = -1;
    }

    for (int remote_id : remote_ids_)
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
    for (int i = 0; i < PCIV_MAX_CHIPNUM; i++)
    {
        remote_fds_[i].resize(MaxPortNum);
        for (int j = 0; j < MaxPortNum; j++)
            if (remote_fds_[i][j] != -1)
                close(remote_fds_[i][j]);
    }

    init_ = false;
}

int PCIVComm::WaitConn(int remote_id)
{
    int ret;

    const char *dev = "/dev/mcc_userdev";
    int fd = open(dev, O_RDWR);
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

int PCIVComm::OpenPort(int remote_id, int port, std::vector<std::vector<int>> &remote_fds)
{
    int ret;

    const char *dev = "/dev/mcc_userdev";
    int fd = open(dev, O_RDWR | O_NONBLOCK);
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

int PCIVComm::EnumChip(int &local_id, std::vector<int> &remote_ids)
{
    int ret;

    const char *dev = "/dev/mcc_userdev";
    int fd = open(dev, O_RDWR);
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
    for (int i = 0; i < HISI_MAX_MAP_DEV; i++)
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

int PCIVComm::Send(int remote_id, int port, uint8_t *data, int len)
{
    if (!init_)
        return KUnInitialized;

    int ret;
    int fd = remote_fds_[remote_id][port];

    int rest_len = len;
    int offset = 0;

    while (rest_len)
    {
    again:
        ret = write(fd, data + offset, len);
        if (ret < 0 && errno == EINTR)
        {
            goto again;
        }
        else
        {
            log_e("write failed,%s", strerror(errno));
            return KSystemError;
        }
        offset += ret;
        rest_len -= ret;
    }
    return KSuccess;
}

int PCIVComm::Recv(int remote_id, int port, uint8_t *data, int len)
{
    if (!init_)
        return KUnInitialized;
    int ret;
    int fd = remote_fds_[remote_id][port];
    ret = read(fd, data, len);
    return ret;
}

const std::vector<int> &PCIVComm::GetRemoteIds()
{
    return remote_ids_;
}

int PCIVComm::GetTransReadPort()
{
    return TransReadPort;
}

int PCIVComm::GetTransWritePort()
{
    return TransWritePort;
}

int PCIVComm::GetCMDPort()
{
    return CommCMDPort;
}

} // namespace rs