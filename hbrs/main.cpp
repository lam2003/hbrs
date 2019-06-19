#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <unistd.h>

#include "system/vi.h"
#include "system/mpp.h"

int main(int argc, char **argv)
{
    rs::MPPSystem::Instance()->Initialize({CAPTURE_MODE_1080P, 5});

    rs::VideoInput vi1;
    vi1.Initialize({2, 4, CAPTURE_MODE_1080P});

    rs::VideoInput vi2;
    vi2.Initialize({4, 8, CAPTURE_MODE_1080P});


    sleep(1000);
    return 0;
}