#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <unistd.h>

#include "system/vi.h"
#include "system/mpp.h"
#include "common/utils.h"

int main(int argc, char **argv)
{
	rs::MPPSystem::Instance()->Initialize({CAPTURE_MODE_1080P, 5});

	rs::VideoInput vi1;
	vi1.Initialize({2, 4, CAPTURE_MODE_1080P});

	// rs::VideoInput vi2;
	// vi2.Initialize({4, 8, CAPTURE_MODE_1080P});

	rs::VideoProcess vpss1;
	vpss1.Initialize({0, CAPTURE_MODE_1080P});

	sleep(2);

	rs::MPPSystem::Bind(vi1, vpss1);

	vpss1.SetEncodeChnSize({640, 480});
	vpss1.SetPIPChnSize({640, 480});
	sleep(5);

	vpss1.SetEncodeChnSize({960, 540});
	//   rs::MPPSystem::UnBind(vi1, vpss1);

	std::thread([]() {
		uint64_t tt = 0;
		while (true)
		{
			uint64_t now = rs::Utils::GetSteadyMilliSeconds();

			if (tt != 0)
			{
				log_e("%llu\n", now - tt);
			}

			tt = now;
			usleep(40000);
		}
	})
		.detach();

	sleep(1000);
	return 0;
}
