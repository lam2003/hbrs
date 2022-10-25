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

#if 1
SerialManager::SerialManager() : ev_(nullptr),
                                 listener_(nullptr),
                                 init_(false)
{
}

#else
SerialManager::SerialManager() : fd_(0),
                                 ev_(nullptr),
                                 listener_(nullptr),
                                 init_(false)
{
}

#endif

SerialManager::~SerialManager()
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

void SerialManager::OnRead(evutil_socket_t sockfd)
{
    memset(buf, 0, sizeof(buf));

again:
    int ret = read(sockfd, buf, sizeof(buf));
    if (ret < 0 && errno == EINTR)
    {
        goto again;
    }
    else if (ret < 0)
    {
        log_e("read failed,%s", strerror(errno));
        return;
    }

    std::vector<int> int_arr;
    for (int i = 0; i < ret; i++)
        int_arr.push_back(buf[i]);

    std::string temp;
    Utils::HexInt2String(int_arr, temp);
    log_d("[on_read]switch command:%s", temp.c_str());

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

int SerialManager::Initialize(event_base *base)
{
    if (init_)
        return KUnInitialized;

    int ret;

    log_d("SERIAL_MANAGER start");

    const char *serial_dev = "/dev/ttyUSB0";
#if 1
    ret = VISCA_open_serial(&interface_, serial_dev);
    if (ret != KSuccess)
    {
        log_e("open %s failed,%s", serial_dev, strerror(errno));
        return KSystemError;
    }
    ev_ = event_new(base, interface_.port_fd, EV_READ | EV_PERSIST, Invoke<SerialManager>, (void *)this);
#else
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

    ev_ = event_new(base, fd_, EV_READ | EV_PERSIST, Invoke<SerialManager>, (void *)this);
#endif

    event_add(ev_, NULL);

    init_ = true;
    return KSuccess;
}

void SerialManager::Close()
{
    if (!init_)
        return;

    log_d("SERIAL_MANAGER stop");

    event_free(ev_);
#if 1
    VISCA_close_serial(&interface_);
#else
    close(fd_);
#endif
    listener_ = nullptr;
    init_ = false;
}

void SerialManager::SetEventListener(std::shared_ptr<SwitchEventListener> listener)
{
    listener_ = listener;
}

int SerialManager::CameraControl(int camera_addr, SerialManager::Command cmd, int value)
{
    if (!init_)
        return KUnInitialized;
    log_d("[camera_control]camera_addr:%d,cmd:%d,value:%d", camera_addr, cmd, value);

    int camera_num;
    int pan_speed, tilt_speed, zoom_speed, zoom_value, channel;

    pan_speed = tilt_speed = zoom_speed = zoom_value = channel = value;

    if (tilt_speed < 1) // tilt_speed range 1 ~ 18
        tilt_speed = 1;
    else if (tilt_speed > 18)
        tilt_speed = 18;

    if (pan_speed < 1) // pan_speed range 1 ~ 14
        pan_speed = 1;
    else if (pan_speed > 14)
        pan_speed = 14;

    if (zoom_speed < 2) // zoom_speed range 2 ~ 7
        zoom_speed = 2;
    else if (zoom_speed > 7)
        zoom_speed = 7;

    if (zoom_value < 0) // far
        zoom_value = 0;
    else if (zoom_value > 0x3ff) // near
        zoom_value = 0x3ff;

    if (channel < 0)
        channel = 0;
    else if (channel > 0x3f)
        channel = 0x3f;

    interface_.broadcast = 0;
    VISCA_set_address(&interface_, &camera_num);

    camera_.address = camera_addr;
    VISCA_clear(&interface_, &camera_);

    switch (cmd)
    {
    case RESET:
        VISCA_set_pantilt_reset(&interface_, &camera_);
        break;
    case STOP:
        VISCA_set_pantilt_stop(&interface_, &camera_, pan_speed, tilt_speed);
        break;
    case UP:
        VISCA_set_pantilt_up(&interface_, &camera_, pan_speed, tilt_speed);
        break;
    case DOWN:
        VISCA_set_pantilt_down(&interface_, &camera_, pan_speed, tilt_speed);
        break;
    case LEFT:
        VISCA_set_pantilt_left(&interface_, &camera_, pan_speed, tilt_speed);
        break;
    case RIGHT:
        VISCA_set_pantilt_right(&interface_, &camera_, pan_speed, tilt_speed);
        break;
    case ZOOM:
        VISCA_set_zoom_value(&interface_, &camera_, zoom_value);
        break;
    case SET_ZOOM_SPEED:
        VISCA_set_zoom_tele_speed(&interface_, &camera_, zoom_speed);
        VISCA_set_zoom_wide_speed(&interface_, &camera_, zoom_speed);
        break;
    case SET_MEMORY:
        VISCA_memory_reset(&interface_, &camera_, channel);
        VISCA_memory_set(&interface_, &camera_, channel);
        break;
    case DEL_MEMORY:
        VISCA_memory_reset(&interface_, &camera_, channel);
        break;
    case LOAD_MEMORY:
        VISCA_memory_recall(&interface_, &camera_, channel);
        break;
    default:
        break;
    }

    return KSuccess;
}
} // namespace rs