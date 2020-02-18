#include "tracker.h"
#include <assert.h>
#include "selectROI.h"

void run()
{
	cv::VideoCapture cap;
	cap.open("save.avi");

	cv::VideoWriter save;
	save.open("save_1.avi", CV_FOURCC('M','P','4','2'), 25.0, cv::Size(480,360), true);
	mosseTracker track;
	selectROI box;
	box.init_param();
	cv::Mat frame;
	bool init = true;
	cv::Rect roi;
	std::string trackingWindow = "tracking.jpg";
	while(cap.read(frame))
	{
		if(init)
		{
			roi = box.add(trackingWindow, frame);
			track.init(roi, frame);
			init = false;
		}
		else 
			roi = track.update(frame);
		cv::rectangle(frame, roi, cv::Scalar(255,255,0));
		cv::imshow(trackingWindow, frame);
		save << frame;
		cv::waitKey(20);
	}
	cap.release();
	box.exit();
}

void img2avi(char* address)
{
	assert(address != NULL);
	char buf[128];
	int nums = 1;
	sprintf(buf, "%s/%04d.jpg", address, nums);

	cv::Mat frame = cv::imread(buf);

	while(frame.data == NULL)
	{
		nums++;
		printf("num = %d\n", nums);
		sprintf(buf, "%s/%04d.jpg", address, nums);
		frame = cv::imread(buf);
	}

	cv::VideoWriter save;
	save.open("save.avi", CV_FOURCC('M','P','4','2'), 25.0, frame.size(), true);
	save << frame;
	nums++;

	while(frame.data != NULL)
	{
		sprintf(buf, "%s/%04d.jpg", address, nums++);
		frame = cv::imread(buf);
		save << frame;
	}
	save.release();
	printf("save over!\n");
}

int main()
{
	//img2avi((char*)"img");
	run();
	return 0;
}