#pragma once

#include "global.h"

struct rtc_time
{
    int tm_sec;   // seconds after the minute -- [0,61]
    int tm_min;   // minutes after the hour   -- [0,59]
    int tm_hour;  // hours after midnight     -- [0,23]
    int tm_mday;  // day of the month         -- [1,31]
    int tm_mon;   // months since January     -- [0,11]
    int tm_year;  // years since 1900
    int tm_wday;  // days since Sunday        -- [0,6]
    int tm_yday;  // days since January 1     -- [0,365]
    int tm_isdst; // Daylight Savings Time flag
};

#define RTC_SET_TIME _IOW('p', 0x0a, rtc_time)
#define RTC_RD_TIME _IOR('p', 0x09, rtc_time)

namespace rs
{
class RTC
{
public:
    static void SetSystemTime(struct tm *p)
    {
        char buf[512];
        sprintf(buf, "date -s %04d.%02d.%02d-%02d:%02d:%02d > /dev/null", p->tm_year + 1900,
                p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
        system(buf);
    }

    static void LoadTime()
    {
        const char *dev = "/dev/rtc";
        int fd = open(dev, O_WRONLY);
        if (fd <= 0)
        {
            log_e("open %s failed,%s", dev, strerror(errno));
            return;
        }

        rtc_time ht;
        ioctl(fd, RTC_RD_TIME, &ht);
        SetSystemTime((struct tm *)&ht);
        close(fd);
    }

    static void SaveTime()
    {
        const char *dev = "/dev/rtc";
        int fd = open(dev, O_WRONLY);
        if (fd <= 0)
        {
            log_e("open %s failed,%s", dev, strerror(errno));
            return;
        }
        time_t t = time(NULL);
        struct tm now_tm = *localtime(&t);

        ioctl(fd, RTC_SET_TIME, &now_tm);
        close(fd);
    }
};
} // namespace rs