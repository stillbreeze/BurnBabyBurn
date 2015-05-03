#include <sstream>
#include <string>
#include <iostream>
#include <opencv\highgui.h>
#include <opencv\cv.h>
#include <Windows.h>
#include <opencv2\video\background_segm.hpp>
#include <opencv2\photo\photo.hpp>
#define _CRT_SECURE_NO_DEPRECATE

using namespace cv;

//initial min and max HSV filter values.
//these will be changed using trackbars
int H_MIN = 0;
int H_MAX = 256;
int S_MIN = 0;
int S_MAX = 256;
int V_MIN = 0;
int V_MAX = 256;

//Alarm trackbar
int ALARM = 0;

//Alarm trigger
bool found = false;

//default capture width and height
const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;

//max number of objects to be detected in frame
const int MAX_NUM_OBJECTS = 50;

//minimum and maximum object area
const int MIN_OBJECT_AREA = 20 * 20;
const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH / 1.5;

//names that will appear at the top of each window
const string windowName = "Original Image";
const string windowName1 = "HSV Image";
const string windowName2 = "Thresholded Image";
const string windowName3 = "After Morphological Operations";
const string trackbarWindowName = "Trackbars";
const string windowName4 = "Intersected Image";

// Open the fire alarm file when triggered
Mat fire;
VideoCapture cap("alarm.mp4");

//ROI image
Mat image_roi;

//This function gets called whenever a
// trackbar position is changed
void on_trackbar(int, void*)
{





}

string intToString(int number){


	std::stringstream ss;
	ss << number;
	return ss.str();
}

//create window for trackbars
void createTrackbars(){

	namedWindow(trackbarWindowName, 0);

	//create memory to store trackbar name on window
	char TrackbarName[60];
	sprintf(TrackbarName, "H_MIN", H_MIN);
	sprintf(TrackbarName, "H_MAX", H_MAX);
	sprintf(TrackbarName, "S_MIN", S_MIN);
	sprintf(TrackbarName, "S_MAX", S_MAX);
	sprintf(TrackbarName, "V_MIN", V_MIN);
	sprintf(TrackbarName, "V_MAX", V_MAX);
	sprintf(TrackbarName, "ALARM", ALARM);

	//create trackbars and insert them into window
	//3 parameters are: the address of the variable that is changing when the trackbar is moved(eg.H_LOW),
	//the max value the trackbar can move (eg. H_HIGH), 
	//and the function that is called whenever the trackbar is moved(eg. on_trackbar)
	//                                  ---->    ---->     ---->      
	createTrackbar("H_MIN", trackbarWindowName, &H_MIN, H_MAX, on_trackbar);
	createTrackbar("H_MAX", trackbarWindowName, &H_MAX, H_MAX, on_trackbar);
	createTrackbar("S_MIN", trackbarWindowName, &S_MIN, S_MAX, on_trackbar);
	createTrackbar("S_MAX", trackbarWindowName, &S_MAX, S_MAX, on_trackbar);
	createTrackbar("V_MIN", trackbarWindowName, &V_MIN, V_MAX, on_trackbar);
	createTrackbar("V_MAX", trackbarWindowName, &V_MAX, V_MAX, on_trackbar);

	//Switch to turn on the alarm system after fixing HSV
	createTrackbar("SWITCH", trackbarWindowName, &ALARM, 1, on_trackbar);

}

//use some of the openCV drawing functions to draw crosshairs
//on your tracked image
void drawObject(int x, int y, Mat &frame){


	circle(frame, Point(x, y), 20, Scalar(0, 255, 0), 2);
	if (y - 25>0)
		line(frame, Point(x, y), Point(x, y - 25), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(x, 0), Scalar(0, 255, 0), 2);
	if (y + 25<FRAME_HEIGHT)
		line(frame, Point(x, y), Point(x, y + 25), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(x, FRAME_HEIGHT), Scalar(0, 255, 0), 2);
	if (x - 25>0)
		line(frame, Point(x, y), Point(x - 25, y), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(0, y), Scalar(0, 255, 0), 2);
	if (x + 25<FRAME_WIDTH)
		line(frame, Point(x, y), Point(x + 25, y), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(FRAME_WIDTH, y), Scalar(0, 255, 0), 2);

	putText(frame, intToString(x) + "," + intToString(y), Point(x, y + 30), 1, 1, Scalar(0, 255, 0), 2);

}

//create structuring element that will be used to "dilate" and "erode" image.
void morphOps(Mat &thresh){

	//the element chosen here is a 3px by 3px rectangle
	Mat erodeElement = getStructuringElement(MORPH_RECT, Size(3, 3));

	//dilate with larger element so make sure object is nicely visible
	Mat dilateElement = getStructuringElement(MORPH_RECT, Size(8, 8));

	erode(thresh, thresh, erodeElement);
	erode(thresh, thresh, erodeElement);


	dilate(thresh, thresh, dilateElement);
	dilate(thresh, thresh, dilateElement);



}

//Tracking of object
void trackFilteredObject(int &x, int &y, Mat threshold, Mat &cameraFeed){

	Mat temp;
	threshold.copyTo(temp);

	//these two vectors needed for output of findContours
	vector< vector<Point> > contours;
	vector<Vec4i> hierarchy;

	//find contours of filtered image using openCV findContours function
	findContours(temp, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);

	//use moments method to find our filtered object
	double refArea = 0;
	bool objectFound = false;
	if (hierarchy.size() > 0) {
		int numObjects = hierarchy.size();

		//if number of objects greater than MAX_NUM_OBJECTS we have a noisy filter
		if (numObjects<MAX_NUM_OBJECTS){
			for (int index = 0; index >= 0; index = hierarchy[index][0]) {

				Moments moment = moments((cv::Mat)contours[index]);
				double area = moment.m00;

				//if the area is less than 20 px by 20px then it is probably just noise
				//if the area is the same as the 3/2 of the image size, probably just a bad filter
				//we only want the object with the largest area so we safe a reference area each
				//iteration and compare it to the area in the next iteration.
				if (area>MIN_OBJECT_AREA && area<MAX_OBJECT_AREA && area>refArea){
					x = moment.m10 / area;
					y = moment.m01 / area;
					objectFound = true;
					refArea = area;
					//alarm trigger
					found = true;

				}
				else
				{
					objectFound = false;
					found = false;
				}


			}
			//let user know you found an object
			if (objectFound == true){
				putText(cameraFeed, "Tracking Object", Point(0, 50), 2, 1, Scalar(0, 255, 0), 2);

				//draw object location on screen
				drawObject(x, y, cameraFeed);
			}

		}
		else putText(cameraFeed, "TOO MUCH NOISE! ADJUST FILTER", Point(0, 50), 1, 2, Scalar(0, 0, 255), 2);
	}
}

//Fire Alarm
void alarm()
{

	//Alarm function
	if (found && ALARM)
	{
		// read next frame
		cap.read(fire);
		imshow("fire alarm", fire);
	}
}

void extractor(Mat &frame, Mat &originalpicture)
{
	vector<vector<Point> > v;
	// Finds contours
	findContours(frame, v, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

	// Finds the contour with the largest area
	int area = 0;
	int idx;
	for (int i = 0; i<v.size(); i++) {
		if (area < v[i].size())
		{
			idx = i;
			area = v[i].size();
		}
	}

	// Calculates the bounding rect of the largest area contour
	Rect rect = boundingRect(v[idx]);
	Point pt1, pt2;
	pt1.x = rect.x;
	pt1.y = rect.y;
	pt2.x = rect.x + rect.width;
	pt2.y = rect.y + rect.height;  

	Rect region_of_interest = Rect(pt1.x, pt1.y, pt2.x, pt2.y);
	image_roi = originalpicture(region_of_interest);
}

int main(int argc, char* argv[])
{
	//some boolean variables for different functionality within this
	//program
	bool trackObjects = true;
	bool useMorphOps = true;

	//Matrix to store each frame of the webcam feed
	Mat cameraFeed;

	//matrix storage for HSV image
	Mat HSV;

	//matrix storage for binary threshold HSV image
	Mat threshold;

	//matrix storing image after morphological operations
	Mat threshold3;

	//Matrix for current frame
	Mat image;

	//GMM output matrix
	Mat gmm;

	//Matrix storing dual operated frames
	Mat dual;

	//x and y values for the location of the object
	int x = 0, y = 0, counter = 0;

	//GMM object
	BackgroundSubtractorMOG2 mog;

	//create slider bars for HSV filtering
	createTrackbars();

	//video capture object to acquire webcam feed
	VideoCapture capture;

	//open capture object at location zero (default location for webcam)
	capture.open(0);

	//set height and width of capture frame
	capture.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
	capture.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);

	//start an infinite loop where webcam feed is copied to cameraFeed matrix
	//all of our operations will be performed within this loop
	while (1){
		//store image to matrix
		capture.read(cameraFeed);

		//convert frame from BGR to HSV colorspace
		cvtColor(cameraFeed, HSV, COLOR_BGR2HSV);

		//filter HSV image between values and store filtered image to
		//threshold matrix
		inRange(HSV, Scalar(H_MIN, S_MIN, V_MIN), Scalar(H_MAX, S_MAX, V_MAX), threshold);
		inRange(HSV, Scalar(H_MIN, S_MIN, V_MIN), Scalar(H_MAX, S_MAX, V_MAX), threshold3);


		//GMM detection
		capture >> image;
		mog(image, gmm, 0.03);
		mog.set("history", 3000);
		mog.set("nmixtures", 5);
		mog.set("backgroundRatio", 0.7);
		mog.set("detectShadows", 0);

		//perform morphological operations on thresholded image to eliminate noise
		//and emphasize the filtered object(s)
		if (useMorphOps)
			morphOps(threshold3);

		//AND operation
		bitwise_and(threshold3, gmm, dual);

		//Perform morphological operations on the intersected image
		if (useMorphOps)
			morphOps(dual);

		//pass in thresholded frame to our object tracking function
		//this function will return the x and y coordinates of the
		//filtered object
		if (trackObjects)
			trackFilteredObject(x, y, dual, cameraFeed);

		//ROI extraction
		if (found && ALARM)
		{
			extractor(dual, cameraFeed);
			cvNamedWindow("ROI", CV_WINDOW_AUTOSIZE);
			imshow("ROI", image_roi);
		}       

		//Check alarm
		alarm();

		//Color Detection frames 
		imshow(windowName2, threshold);
		imshow(windowName, cameraFeed);
		imshow(windowName1, HSV);
		imshow(windowName3, threshold3);

		//GMM extracted frames
		imshow("Extracted Image", gmm);

		//Intersected frame
		imshow(windowName4, dual);

		//delay 30ms so that screen can refresh.
		//image will not appear without this waitKey() command
		if (counter == 0)
		{
			waitKey(10000);
			counter++;
		}
		else
			waitKey(30);
	}






	return 0;
}