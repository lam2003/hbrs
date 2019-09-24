#include "global.h"

namespace rs
{
class CPUBind
{
public:
    static void SetCPU(int no)
    {
        cpu_set_t mask;
        cpu_set_t get;

        CPU_ZERO(&mask);
        CPU_SET(no, &mask);

        /* 设置cpu 亲和性(affinity)*/
        if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
        {
            log_e("%d set thread affinity faild", pthread_self());
        }
        else
        {
            log_d("%d set thread affinity successfully", pthread_self());
        }

        CPU_ZERO(&get);
        if (pthread_getaffinity_np(pthread_self(), sizeof(get), &get) < 0)
        {
            log_e("%d get thread affinity failed", pthread_self());
        }

        if (CPU_ISSET(no, &get))
        {
            log_d("%d is running in processor %d", pthread_self(), no);
        }
    }
    static void SetCPUAuto()
    {
        static uint32_t g_InvokeTime = 0;
        static std::mutex g_Mux;

        int cpus = sysconf(_SC_NPROCESSORS_CONF);
        log_d("%d has %d processor(s)", pthread_self(), cpus);

        g_Mux.lock();
        int no = g_InvokeTime++ % cpus;
        g_Mux.unlock();

        SetCPU(no);
    }
};
} // namespace rs