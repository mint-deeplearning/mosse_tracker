#include "tracker.h"
#include <assert.h>
#include <stdlib.h>
#include <time.h>

mosseTracker::mosseTracker()
{
	
}

mosseTracker::~mosseTracker()
{
	
}

cv::Mat mosseTracker::createGaussKernel(cv::Size sz, float sigma, cv::Point center)
{
	cv::Mat gauss = cv::Mat::zeros(sz, CV_32FC1);
	for(int r = 0; r < sz.height; r++)
		for(int c = 0; c < sz.width; c++)
		{
			float v = (r - center.y) * (r - center.y) + (c - center.x) * (c - center.x);
			v /= (2 * sigma);
			gauss.at<float>(r, c) = std::exp(-v);
		}
	return gauss;
}

void mosseTracker::init_param()
{
	_sigma = 100;
	_eta = 0.125;
}

cv::Mat mosseTracker::bgr2gray(const cv::Mat& image)
{
	cv::Mat res;
	int chans = image.channels();
	if(chans == 3)
		cv::cvtColor(image, res, CV_BGR2GRAY);
	else res = image.clone();
	return res;
}

cv::Mat mosseTracker::imcrop(cv::Rect roi, const cv::Mat& image)
{
	cv::Rect img = cv::Rect(0,0,image.cols,image.rows);
	cv::Rect res = roi & img;
	return image(res).clone();
}

cv::Mat mosseTracker::fft(cv::Mat image, bool backwards)
{
	if(image.channels() == 1)
	{
		cv::Mat planes[] = {cv::Mat_<float>(image), cv::Mat_<float>::zeros(image.size())};
		cv::merge(planes, 2, image);
	}
	cv::dft(image, image, backwards ? (cv::DFT_INVERSE | cv::DFT_SCALE) : 0);

	return image;
}

cv::Mat mosseTracker::conj(const cv::Mat& image)
{
	assert(image.channels() == 2);
	cv::Mat mat[2];
	cv::split(image, mat);
	mat[1] *= -1;
	cv::Mat res;
	cv::merge(mat, 2, res);
	return res;
}

cv::Mat mosseTracker::createHanningMats(int rows, int cols)
{
	cv::Mat hann1t = cv::Mat(cv::Size(cols, 1), CV_32F, cv::Scalar(0));
	cv::Mat hann2t = cv::Mat(cv::Size(1, rows), CV_32F, cv::Scalar(0));

	for(int i = 0; i < hann1t.cols; i++)
		hann1t.at<float>(0, i) = 0.5 * (1 - std::cos(2 * CV_PI * i / (hann1t.cols - 1)));
	for(int i = 0; i < hann2t.rows; i++)
		hann2t.at<float>(i, 0) = 0.5 * (1 - std::cos(2 * CV_PI * i / (hann2t.rows - 1)));

	cv::Mat hann2d = hann2t * hann1t;
	return hann2d;
}

cv::Mat mosseTracker::preprocess(const cv::Mat& image)
{
	cv::Mat win = createHanningMats(image.rows, image.cols);
	float eps = 1e-5;
	cv::Mat img = image + cv::Scalar::all(1);
	img = cv::Mat_<float>(img);
	cv::log(img, img);
	cv::Scalar mean, std;
	cv::meanStdDev(img, mean, std);
	img = (img - cv::Scalar::all(mean[0])) / (std[0] + eps);
	return img.mul(win);
}

cv::Mat mosseTracker::rand_warp(const cv::Mat& image)
{
	srand((unsigned)time(NULL));
	float a = -180.0 / 16;
	float b = -a;
	float rand_v_1 = rand() % 101 / 100.0;
	float r = a + (b - 1) * rand_v_1;

	float rand_v_2 = rand() % 101 / 100.0;
	float scale = 1 - 0.1 + 0.2 * rand_v_2;

	cv::Mat rotate_image = cv::Mat::zeros(image.size(), image.type());
	cv::Point center = cv::Point(image.cols/2, image.rows/2);
	cv::Mat rot_mat = cv::getRotationMatrix2D(center, double(r), double(scale));
	cv::warpAffine(image, rotate_image, rot_mat, image.size());

	return rotate_image;
}

cv::Mat mosseTracker::complexMultiplication(cv::Mat a, cv::Mat b)
{
	std::vector<cv::Mat> pa;
	std::vector<cv::Mat> pb;
	cv::split(a, pa);
	cv::split(b, pb);

	std::vector<cv::Mat> pres;
	pres.push_back(pa[0].mul(pb[0]) - pa[1].mul(pb[1]));
	pres.push_back(pa[0].mul(pb[1]) + pa[1].mul(pb[0]));

	cv::Mat res;
	cv::merge(pres, res);

	return res;
}

void mosseTracker::init(cv::Rect roi, const cv::Mat& image)
{
	init_param();
	_roi = roi;

	cv::Point center = cv::Point(roi.x+roi.width/2, roi.y+roi.height/2);

	guassKernelMatrix = createGaussKernel(image.size(), _sigma, center);

	cv::Mat gray = bgr2gray(image);

	cv::Mat gray_crop = imcrop(roi, gray);

	cv::Mat guassKernelMatrix_crop = imcrop(roi, guassKernelMatrix);

	init_sz.width = guassKernelMatrix_crop.cols;
	init_sz.height = guassKernelMatrix_crop.rows;

	gauss_fft = fft(guassKernelMatrix_crop);

	if(gray_crop.size() != guassKernelMatrix_crop.size())
		cv::resize(gray_crop, gray_crop, guassKernelMatrix_crop.size());

	fi = preprocess(gray_crop);

	fi_fft = fft(fi);

	Ai = complexMultiplication(gauss_fft, conj(fi_fft));
	Bi = complexMultiplication(fi_fft, conj(fi_fft));

	int N = 128;
	for(int i = 0; i < N; i++)
	{
		fi = preprocess(rand_warp(gray_crop));
		fi_fft = fft(fi);
		Ai += complexMultiplication(gauss_fft, conj(fi_fft));
		Bi += complexMultiplication(fi_fft, conj(fi_fft));
	}

	Ai *= _eta;
	Bi *= _eta;
}

cv::Mat mosseTracker::convert(const cv::Mat& src)
{
	cv::Mat cv8uc1 = cv::Mat::zeros(src.size(), CV_8UC1);
	for(int r = 0; r < src.rows; r++)
		for(int c = 0; c < src.cols; c++)
		{
			float val = src.at<float>(r,c);
			val = val > 255 ? 255 : val;
			cv8uc1.at<uchar>(r,c) = (unsigned char)val;
		}
	return cv8uc1;
}

cv::Mat mosseTracker::real(cv::Mat image)
{
	std::vector<cv::Mat> mats;
	cv::split(image, mats);
	return mats[0];
}

cv::Mat mosseTracker::imag(cv::Mat image)
{
	std::vector<cv::Mat> mats;
	cv::split(image, mats);
	return mats[1];
}

cv::Mat mosseTracker::complexDivision(cv::Mat a, cv::Mat b)
{
	std::vector<cv::Mat> pa;
	std::vector<cv::Mat> pb;

	cv::split(a, pa);
	cv::split(b, pb);

	cv::Mat divisior = 1.0 / (pb[0].mul(pb[0]) + pb[1].mul(pb[1]));

	std::vector<cv::Mat> pres;

	pres.push_back((pa[0].mul(pb[0]) + pa[1].mul(pb[1])).mul(divisior));
	pres.push_back((pa[1].mul(pb[0]) + pa[0].mul(pb[1])).mul(divisior));

	cv::Mat res;
	cv::merge(pres, res);
	return res;
}

cv::Rect mosseTracker::update(const cv::Mat& image)
{
	cv::Mat gray = bgr2gray(image);
	Hi = complexDivision(Ai, Bi);

	fi = imcrop(_roi, gray);
	//cv::resize(_roi, gray);
	cv::resize(fi, fi, init_sz);
	fi = preprocess(fi);

	cv::Mat response = fft(complexMultiplication(Hi, fft(fi)), true);

	cv::normalize(response, response, 0, 1, cv::NORM_MINMAX);

	response *= 255.0;

	cv::Mat resp = real(response);
	cv::Mat resp_cv8u = cv::Mat_<unsigned char>(resp);

	cv::Point ps;
	double max_response;
	cv::minMaxLoc(resp_cv8u, NULL, &max_response, NULL, &ps);

	float dx = ps.x - init_sz.width / 2;
	float dy = ps.y - init_sz.height / 2;

	_roi = cv::Rect(_roi.x + dx, _roi.y + dy, init_sz.width, init_sz.height);

	train(gray);

	return _roi;
}

void mosseTracker::train(const cv::Mat& image)
{
	fi = imcrop(_roi, image);
	cv::resize(fi, fi, init_sz);
	fi = preprocess(fi);

	fi_fft = fft(fi);
	Ai = _eta * complexMultiplication(gauss_fft, conj(fi_fft)) + (1 - _eta) * Ai;
	Bi = _eta * complexMultiplication(fi_fft, conj(fi_fft)) + (1 - _eta) * Bi;
}