#include "selectROI.h"

selectROI::selectROI()
{
}

selectROI::~selectROI()
{
}

void selectROI_handler::onMouse(int event, int x, int y, int flags, void* param)
{
	selectROI_handler_param* shparam = (selectROI_handler_param*)param;
	switch (event)
	{
	case CV_EVENT_LBUTTONDOWN:
		shparam->drawRoi = true;
		shparam->click_1.x = x;
		shparam->click_1.y = y;
		break;
	case CV_EVENT_MOUSEMOVE:
		shparam->click_move.x = x;
		shparam->click_move.y = y;
		break;
	case CV_EVENT_LBUTTONUP:
		shparam->click_2.x = x;
		shparam->click_2.y = y;
		shparam->drawRoi = false;
		shparam->endDraw = true;
		break;
	default:
		break;
	}
}

cv::Rect selectROI::add(const std::string windname, const cv::Mat& image)
{
	
	cv::Mat src_copy = image.clone();
	cv::imshow(windname, src_copy);
	int lx, ly, w, h;
	while (!param->endDraw)
	{
		cv::setMouseCallback(windname, selectROI_handler::onMouse, param);
		src_copy = image.clone();
		cv::Rect temp;
		if (param->drawRoi)
		{		
			lx = param->click_1.x > param->click_move.x ? param->click_move.x : param->click_1.x;
			ly = param->click_1.y > param->click_move.y ? param->click_move.y : param->click_1.y;
			w = std::abs(param->click_move.x - param->click_1.x);
			h = std::abs(param->click_move.y - param->click_1.y);
			temp = cv::Rect(lx, ly, w, h);
			cv::rectangle(src_copy, temp, cv::Scalar(255,255,255));
		}
		cv::imshow(windname, src_copy);
		cv::waitKey(20);
	}
	lx = param->click_1.x > param->click_2.x ? param->click_2.x : param->click_1.x;
	ly = param->click_1.y > param->click_2.y ? param->click_2.y : param->click_1.y;
	w = std::abs(param->click_2.x - param->click_1.x);
	h = std::abs(param->click_2.y - param->click_1.y);
	cv::Rect res = cv::Rect(lx, ly, w, h);
	return res;
}

void selectROI::init_param()
{
	param = new selectROI_handler_param();
	memset(param, 0, sizeof(selectROI_handler_param));
	param->drawRoi = false;
	param->endDraw = false;
}

void selectROI::exit()
{
	delete param;
}

void selectROI::reinit()
{
	param->drawRoi = false;
	param->endDraw = false;
}