#pragma once
#ifndef _SELECT_ROI_H_HH
#define _SELECT_ROI_H_HH

#include <opencv2/opencv.hpp>
#include <map>

typedef struct selectROI_handler_param {
	cv::Point click_1, click_2, click_move;
	bool drawRoi;
	bool endDraw;
}selectROI_handler_param;

class selectROI_handler
{
	friend class selectROI;
protected:
	static void onMouse(int event, int x, int y, int flags, void* param);
};

class selectROI {
public:
	selectROI();
	virtual ~selectROI();
public:
	void init_param();
	cv::Rect add(const std::string windname, const cv::Mat& image);
	void exit();
	void reinit();
private:
	selectROI_handler_param* param;
};
#endif
