#pragma once

#include "global.h"

namespace rs
{
static void Daemon()
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
} // namespace rs