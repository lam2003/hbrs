#include "common/switch.h"
#include "common/err_code.h"

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

Switch::Switch() : init_(false)
{
}

Switch::~Switch()
{
}

void Switch::OnRead(evutil_socket_t sockfd)
{
    memset(buf, 0, sizeof(buf));
    int ret = read(sockfd, buf, sizeof(buf));
    if (ret > 0)
    {
        // std::string str(buf, len);
        
    }
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
    init_ = false;
}

} // namespace rs