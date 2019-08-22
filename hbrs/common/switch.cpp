#include "common/switch.h"
#include "common/err_code.h"
#include "common/utils.h"
#include "common/config.h"

namespace rs
{

template <typename T>
static void Invoke(evutil_socket_t sockfd, short event, void *arg)
{
    if (event == EV_READ)
    {
        T *obj = static_cast<T *>(arg);
        obj->OnRead(sockfd);
    }
}

Switch::Switch() : fd_(0),
                   ev_(nullptr),
                   listener_(nullptr),
                   init_(false)
{
}

Switch::~Switch()
{
}

bool CompareCommand(const std::vector<int> &hex_int_arr, const uint8_t *data, int len)
{
    if (len != static_cast<int>(hex_int_arr.size()))
        return false;

    for (size_t i = 0; i < hex_int_arr.size(); i++)
    {
        if (data[i] != static_cast<char>(hex_int_arr[i]))
            return false;
    }

    return true;
}

void Switch::OnRead(evutil_socket_t sockfd)
{
    memset(buf, 0, sizeof(buf));
    int ret = read(sockfd, buf, sizeof(buf));

    if (ret <= 0)
    {
        log_e("read ret <= 0");
        return;
    }
    std::ostringstream oss;
    for (int i = 0; i < ret; i++)
        oss << std::hex << buf[i] << " ";
    log_e("switch command:%s", oss.str().c_str());

    RS_SCENE scene;
    if (CompareCommand(Config::Instance()->switch_cmd_.tea_fea, buf, ret))
    {
        log_d("switch to teacher feature");
        scene = TEA_FEA;
    }
    else if (CompareCommand(Config::Instance()->switch_cmd_.stu_fea, buf, ret))
    {
        log_d("switch to student feature");
        scene = STU_FEA;
    }
    else if (CompareCommand(Config::Instance()->switch_cmd_.tea_full, buf, ret))
    {
        log_d("switch to teacher fullview");
        scene = TEA_FULL;
    }
    else if (CompareCommand(Config::Instance()->switch_cmd_.stu_full, buf, ret))
    {
        log_d("switch to student fullview");
        scene = STU_FULL;
    }
    else if (CompareCommand(Config::Instance()->switch_cmd_.bb_fea, buf, ret))
    {
        log_d("switch to black board feature");
        scene = BB_FEA;
    }
    else if (CompareCommand(Config::Instance()->switch_cmd_.pc_capture, buf, ret))
    {
        log_d("switch to pc capture");
        scene = PC_CAPTURE;
    }
    else
    {
        log_e("unknow command");
        return;
    }

    if (listener_ != nullptr)
        listener_->OnSwitchEvent(scene);
}

int Switch::Initialize(event_base *base)
{
    if (init_)
        return KUnInitialized;

    int ret;

    const char *serial_dev = "/dev/ttyUSB0";
    int fd_ = open(serial_dev, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd_ < 0)
    {
        log_e("open %s failed,%s", serial_dev, strerror(errno));
        return KSystemError;
    }

    termios termios;
    memset(&termios, 0, sizeof(termios));
    tcgetattr(fd_, &termios);
    termios.c_cflag |= (CLOCAL | CREAD);
    termios.c_cflag &= ~CSIZE;
    termios.c_cflag &= ~CRTSCTS;
    termios.c_cflag |= CS8;
    termios.c_cflag &= ~CSTOPB;
    termios.c_iflag &= IGNPAR;
    termios.c_oflag = 0;
    termios.c_lflag = 0;

    ret = cfsetispeed(&termios, B9600);
    if (ret < 0)
    {
        log_e("cfsetispeed failed");
        return HI_FAILURE;
    }

    ret = cfsetospeed(&termios, B9600);
    if (ret < 0)
    {
        log_e("cfsetispeed failed");
        return HI_FAILURE;
    }

    ret = tcsetattr(fd_, TCSANOW, &termios);
    if (ret < 0)
    {
        log_e("cfsetispeed failed");
        return HI_FAILURE;
    }

    ev_ = event_new(base, fd_, EV_READ | EV_PERSIST, Invoke<Switch>, (void *)this);
    event_add(ev_, NULL);

    init_ = true;
    return KSuccess;
}

void Switch::Close()
{
    if (!init_)
        return;

    event_free(ev_);
    close(fd_);
    listener_ = nullptr;
    init_ = false;
}

void Switch::SetEventListener(SwitchEventListener *listener)
{
    listener_ = listener;
}

} // namespace rs