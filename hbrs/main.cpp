#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <unistd.h>

#include "system/vi.h"
#include "system/mpp.h"
#include "system/vpss.h"
#include "system/vo.h"
#include "common/utils.h"

int main(int argc, char **argv)
{
	rs::MPPSystem::Instance()->Initialize({CAPTURE_MODE_720P, 10});

	rs::VideoInput vi1;
	vi1.Initialize({2, 4, CAPTURE_MODE_720P});

	rs::VideoProcess vpss1;
	vpss1.Initialize({0, CAPTURE_MODE_720P});

	rs::MPPSystem::Bind<HI_ID_VIU, HI_ID_VPSS>(0, 4, 0, 0);
	vpss1.SetChnSize(VPSS_ENCODE_CHN, {1280, 720});
	vpss1.SetChnSize(2, {640, 360});

	rs::VideoOutput vo1;
	vo1.Initialize({VO_HD_DEV, VO_INTF_VGA | VO_INTF_HDMI, VO_OUTPUT_1080P60});
	vo1.StartChn({{0, 0, 1920, 1080}, 0, 0});

	rs::VideoOutput vo2;
	vo2.Initialize({VO_VIR_DEV, 0, VO_OUTPUT_1080P60});
	vo2.StartChn({{0, 0, 1280, 720}, 0, 0});
	vo2.StartChn({{640, 360, 640, 360}, 1, 1});

	rs::VideoProcess vpss2;
	vpss2.Initialize({1, CAPTURE_MODE_1080P});
	rs::MPPSystem::Bind<HI_ID_VOU, HI_ID_VPSS>(10, 0, 1, 0);

	rs::MPPSystem::Bind<HI_ID_VPSS, HI_ID_VOU>(1, 1, 0, 0);

	std::thread([]() {
		int ret;
		VIDEO_FRAME_INFO_S frame;
		ret = HI_MPI_VPSS_SetDepth(0, 1, 1);
		if (ret != KSuccess)
		{
			printf("fuck\n\n\n");
			return;
		}
		// int i = 300;
		while (true)
		{

			ret = HI_MPI_VPSS_UserGetFrame(0, 1, &frame);
			// printf("ret:%#x\n",ret);
			if (ret == KSuccess)
			{
				frame.stVFrame.u64pts = 0;
				// printf("###\n");
				ret = HI_MPI_VO_SendFrame(10, 0, &frame);
				if (ret != 0)
				{
					printf("HI_MPI_VO_SendFrame %#x\n", ret);
				}
				// i--;

				HI_MPI_VPSS_UserReleaseFrame(0, 1,
											 &frame);
			}
			usleep(10000);
		}
	})
		.detach();

	std::thread([]() {
		int ret;
		VIDEO_FRAME_INFO_S frame;
		ret = HI_MPI_VPSS_SetDepth(0, 2, 1);
		if (ret != KSuccess)
		{
			printf("fuck\n\n\n");
			return;
		}
		// int i = 300;
		while (true)
		{

			ret = HI_MPI_VPSS_UserGetFrame(0, 2, &frame);
			// printf("ret:%#x\n",ret);
			if (ret == KSuccess)
			{
				frame.stVFrame.u64pts = 0;
				// printf("###\n");
				ret = HI_MPI_VO_SendFrame(10, 1, &frame);
				if (ret != 0)
				{
					printf("HI_MPI_VO_SendFrame %#x\n", ret);
				}
				// i--;

				HI_MPI_VPSS_UserReleaseFrame(0, 2,
											 &frame);
			}
				usleep(10000);
		}
	})
		.detach();

	while (1)
		sleep(1000);
	return 0;
}
