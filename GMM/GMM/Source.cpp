#include <sstream>
#include <iostream>
#include <opencv\highgui.h>
#include <opencv\cv.h>
#include <Windows.h>
#include <opencv2\video\background_segm.hpp>
#define _CRT_SECURE_NO_DEPRECATE

using namespace cv;

// current gray-level image
Mat gray;
// accumulated background
Mat background;
// background image
Mat backImage;
// foreground image
Mat foreground;
//GMM foreground image
//Mat foreground2;
//GMM output matrix
Mat gmm;
//Standard output mattrix
Mat video;
// learning rate in background accumulation
double learningRate = 0.5;
// threshold for foreground extraction
int threshold;


// processing method for comparison
void process1(cv::Mat &frame, cv::Mat &output) {
	// convert to gray-level image
	cv::cvtColor(frame, gray, CV_BGR2GRAY);
	// initialize background to 1st frame
	if (background.empty())
		gray.convertTo(background, CV_32F);
	// convert background to 8U
	background.convertTo(backImage, CV_8U);
	// compute difference between image and background
	cv::absdiff(backImage, gray, foreground);
	// apply threshold to foreground image
	cv::threshold(foreground, output, 25, 255, cv::THRESH_BINARY_INV);
	// accumulate background
	cv::accumulateWeighted(gray, background, learningRate, output);
}
/*void process2(cv::Mat &frame2, cv::Mat &output2) {
	//Mixture of Gaussian objects
	BackgroundSubtractorMOG mog;
	//Update the background to gmm matrix
	mog.operator()(frame2, output2, 1);
	mog.getBackgroundImage(output2);
	//Thresholding output
	//cv::threshold(output2, output2, 125, 255, cv::THRESH_BINARY_INV);
}*/

int main()
{
	//Matrix for current frame
	Mat image;
	// lock variable for intial delaa of webcam
	int c = 0;
	//GMM object
	BackgroundSubtractorMOG2 mog;
	VideoCapture cap;
	cap.open(0);
	//Original Image
	namedWindow("window", 1);
	//Extracted Image 1
	namedWindow("output", 1);
	//Extracted Image 2
	namedWindow("Extracted Image",1);
	while (1)
	{
		cap >> image;
		//Standard detection
		process1(image, video);
		//GMM detection
		mog(image, gmm, 0.03);
		mog.set("history", 3000);
		mog.set("nmixtures", 5);
		mog.set("backgroundRatio", 0.7);
		mog.set("detectShadows", 0);
		//Original, standard and gmm extracted image
		imshow("window", image);
		imshow("output", video);
		imshow("Extracted Image", gmm);
		//Webcam delay factoring
		if (c == 0)
		{
			waitKey(3000);
			c++;
		}
		else
			waitKey(25);
	}
	return 0;
}