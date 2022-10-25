#include "global.h"
#include "common/system.h"
#include "common/utils.h"
#include "common/config.h"
#include "common/err_code.h"

namespace rs
{
void Daemon()
{
    int pid;

    if ((pid = fork()))
    {
        exit(0);
    }
    else if (pid < 0)
    {
        exit(1);
    }

    setsid();

    if ((pid = fork()))
    {
        exit(0);
    }
    else if (pid < 0)
    {
        exit(1);
    }

    umask(0);
    return;
}

void ConfigLogger()
{
    if (KSuccess != Utils::CreateDir(Config::Instance()->logger_.dir_path))
    {
        log_e("create logger dir failed");
        return;
    }

    std::ostringstream oss;
    oss << Config::Instance()->logger_.dir_path << "/"
        << "rs_" << Utils::GetLocalTime() << ".log";
    setbuf(stdout, NULL);
    elog_init(oss.str().c_str());
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);
    elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_DIR | ELOG_FMT_LINE | ELOG_FMT_TIME | ELOG_FMT_T_INFO | ELOG_FMT_FUNC);
    elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_TIME | ELOG_FMT_T_INFO | ELOG_FMT_FUNC);
    elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_ALL);
    elog_set_text_color_enabled(true);
    elog_start(STRINGIZE_VALUE_OF(RS_VERSION));
    if (Config::Instance()->logger_.auto_clean)
    {
#if 0
        std::thread([]() {
            while (true)
            {
#endif
        DIR *dir;
        dirent *ptr;
        if ((dir = opendir(Config::Instance()->logger_.dir_path.c_str())) == NULL)
        {
            log_e("opendir %s failed,%s", Config::Instance()->logger_.dir_path.c_str(), strerror(errno));
            return;
        }

        while ((ptr = readdir(dir)) != NULL)
        {
            if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
            {
                continue;
            }
            else if (ptr->d_type != DT_REG)
            {
                log_d("skip file type:%d,%s", ptr->d_type, ptr->d_name);
                continue;
            }

            std::string filename = ptr->d_name;
            std::string suffix = filename.substr(filename.find_last_of("."), filename.length());
            if (suffix != ".log")
                continue;

            std::ostringstream oss;
            oss << Config::Instance()->logger_.dir_path.c_str() << "/" << ptr->d_name;

            struct stat buf;
            if (KSuccess != stat(oss.str().c_str(), &buf))
            {
                log_e("stat %s failed,%s", oss.str().c_str(), strerror(errno));
                continue;
            }

            timespec now_ts;
            clock_gettime(CLOCK_REALTIME, &now_ts);

            time_t diff = now_ts.tv_sec - buf.st_mtim.tv_sec;
            log_d("log file:%s,duration:%d seconds", oss.str().c_str(), diff);

            if (diff > Config::Instance()->logger_.clean_duration)
            {
                if (KSuccess != remove(oss.str().c_str()))
                {
                    log_e("remove %s failed,%s", oss.str().c_str(), strerror(errno));
                    continue;
                }
                log_w("remove log file:%s", oss.str().c_str());
            }
        }

        closedir(dir);
#if 0
                sleep(3600);
            }
        })
            .detach();
#endif
    }
}
} // namespace rs