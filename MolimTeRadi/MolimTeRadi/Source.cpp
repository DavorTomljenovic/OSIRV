#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <windows.h>


using namespace std;
using namespace cv;

#define key_D 0x44
#define key_F1 0x70
#define key_F2 0x71
#define key_F3 0x72

HANDLE hStdin;
DWORD fdwSaveOldMode;

bool debug = false;
bool c1Type = 1;
bool c2Type = 0;
bool c3Type = 0;

int frames = 0;
int area = 0;
int height = 0;
int width = 0;
int testArea = 0;

//Ispis pogrešaka u log datoteku
void ErrorExit(LPSTR lpszMessage) {
	fprintf(stderr, "%s\n", lpszMessage);
	SetConsoleMode(hStdin, fdwSaveOldMode);
	ExitProcess(0);
}

//Ukljuèivanje debugginga i postavljanje kanala
void KeyEventProc(KEY_EVENT_RECORD key) {
	if (key.bKeyDown) {

		switch (key.wVirtualKeyCode){
		case key_D:
			debug = !debug;
			break;
		case key_F1:
			c1Type = !c1Type;
			break;
		case key_F2:
			c2Type = !c2Type;
			break;
		case key_F3:
			c3Type = !c3Type;
			break;
		}
	}
}

DWORD WINAPI keyboardThreadFunction(LPVOID lpParam) {
	DWORD cNumRead, fdwMode, i;
	INPUT_RECORD irInBuf[128];

	hStdin = GetStdHandle(STD_INPUT_HANDLE);
	if (hStdin == INVALID_HANDLE_VALUE)
		ErrorExit("GetStdHandle");

	if (!GetConsoleMode(hStdin, &fdwSaveOldMode))
		ErrorExit("GetConsoleMode");

	fdwMode = ENABLE_WINDOW_INPUT;
	if (!SetConsoleMode(hStdin, fdwMode))
		ErrorExit("SetConsoleMode");

	while (1) {
		if (!ReadConsoleInput(
			hStdin, 
			irInBuf, 
			128, 
			&cNumRead)) 
			ErrorExit("ReadConsoleInput");

		for (i = 0; i < cNumRead; i++)  {
			switch (irInBuf[i].EventType)  {
			case KEY_EVENT:
				KeyEventProc(irInBuf[i].Event.KeyEvent);
				break;

			default:
				break;
			}
		}
	}

	return 0;
}

//Simuliranje pritiska tipke
void simulateKeyPress(int no_fingers){
	switch (no_fingers){
	case 1:
		keybd_event(VK_F5, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
		break;
	case 2:
		keybd_event(VK_ESCAPE, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
		break;
	case 4:
		keybd_event(VK_LEFT, 0x45, KEYEVENTF_EXTENDEDKEY | 0, 0);
		break;
	case 5:
		keybd_event(VK_RIGHT,0x45,KEYEVENTF_EXTENDEDKEY | 0,0);
		break;

	}
}

//Odreðivanje podruèja dlana
Mat getConvexHull(Mat foreground, Mat frame){

	vector<vector<Point> > contours;

	vector<vector<Point> > tcontours;

	//Odreðivanje kontura
	findContours(foreground, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

	int no_fingers = 0;
	int keyFrames = 0;

	for (int i = 0; i < contours.size(); i++){

		if (contourArea(contours[i]) >= 8000) //Zanemaruje mala podruèja
		{
			tcontours.push_back(contours[i]);

			//crtanje okvira ruke
			vector<vector<Point> > hulls(1);
			vector<vector<int> > hullsI(1);
			convexHull(Mat(tcontours[0]), hulls[0], false);
			convexHull(Mat(tcontours[0]), hullsI[0], false);
			
			drawContours(frame, hulls, -1, cv::Scalar(0, 255, 0), 2);

			RotatedRect rect = minAreaRect(Mat(tcontours[0]));

			Point2f rect_points[4]; rect.points(rect_points);

			for (int j = 0; j < 4; j++)
				line(frame, rect_points[j], rect_points[(j + 1) % 4], Scalar(255, 0, 0), 1, 8);

			Size2f size = rect.size;

			//VEKTOR ZNACAJKI
			width = size.width;
			height = size.height;
			testArea = width*height;
			///////////////////////

			area += size.width*size.height;
			frames++;
			if (frames == 9){
				area = area / 9;
				frames = 0;

				if (area > 21000 && area < 25000)
					no_fingers = 1;
				else if (area > 18000 && area < 19000)
					no_fingers = 2;
				else if (area > 19000 && area < 30000)
					no_fingers = 3;
				else if (area > 37000 && area < 43000)
					no_fingers = 4;
				else if (area > 44000 && area < 58000)
					no_fingers = 5;

				area = 0;

				simulateKeyPress(no_fingers);
			}

		}
	}

	return frame;
}


int main(int argc, char *argv[])
{
	ofstream dataset;
	dataset.open("HandGestureData.csv");
	dataset << "Width,Height,Area,Gesture\n";
	//POSTAVI RUCNO ZA SVAKU GESTU
	int gesture = 2;

	bool init = false;

	Mat frame;
	Mat channels[3];
	Mat foreground;
	Mat YCRCB;
	Mat temp;

	CascadeClassifier face_cascade;
	face_cascade.load("C:\\haarcascade_frontalface_alt2.xml");

	std::vector<Rect> faces;
	VideoCapture cam(0);

	if (!cam.isOpened()){
		cout << "Error initializing camera" << endl;
		return -1;
	}


	//Prozor s prikazom kamere
	namedWindow("Original", CV_WINDOW_AUTOSIZE);

	HANDLE threads[1];
	threads[0] = CreateThread(NULL, 0, keyboardThreadFunction, 0, 0, 0);

	cout << "Commands:" << endl << "D ->Debug" << endl << "F1 -> Invert channel 1" << endl << "F2 -> Invert channel 2" << endl << "F3 -> Invert channel 3" << endl;
	while (true){

		bool readFrameCheck = cam.read(frame);

		if (readFrameCheck == false){
			break;
		}

		//Detekcija lica
		face_cascade.detectMultiScale(frame, faces, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, Size(30, 30));

		//Promjena formata boje
		cv::cvtColor(frame, foreground, cv::COLOR_BGR2YCrCb);

		//Podjela u 3 zasebna kanala
		split(foreground, channels);

		//Thresholding odvojenih kanala
		cv::inRange(channels[0], cv::Scalar(54), cv::Scalar(163), channels[0]);
		cv::inRange(channels[1], cv::Scalar(130), cv::Scalar(165), channels[1]);
		cv::inRange(channels[2], cv::Scalar(128), cv::Scalar(129), channels[2]);

		threshold(channels[0], channels[0], 0, 255, c1Type);
		threshold(channels[1], channels[1], 0, 255, c2Type);
		threshold(channels[2], channels[2], 0, 255, c3Type);

		//Otklanjanje suma - Morfoloske operacije
		for (int i = 0; i < 3; i++){
			erode(channels[i], channels[i], Mat());
			dilate(channels[i], channels[i], Mat());
			dilate(channels[i], channels[i], Mat());
		}

		//spajanje kanala u jedan
		bitwise_and(channels[0], channels[1], temp);
		bitwise_and(temp, channels[2], foreground);

		//Morfologija
		erode(foreground, foreground, Mat());
		dilate(foreground, foreground, Mat());

		erode(temp, temp, Mat());
		dilate(temp, temp, Mat());

		for (int i = 1; i < 12; i = i + 2)
		{
			GaussianBlur(foreground, foreground, Size(i, i), 0, 0);
		}

		erode(temp, temp, Mat());
		dilate(temp, temp, Mat());

		//Uklanjanje lica
		for (int i = 0; i < faces.size(); i++)
		{
			rectangle(temp, faces[i], CV_RGB(0, 0, 0), CV_FILLED);

		}

		frame = getConvexHull(temp, frame);


		if (debug == true){
			imshow("YCrCb", foreground);
			imshow("Prvi kanal", channels[0]);
			imshow("Drugi kanal", channels[1]);
			imshow("Treci kanal", channels[2]);
			imshow("Temp", temp);
			dataset << width << "," << height << "," << testArea << ","<< gesture<< "\n";
		}

		imshow("Original", frame); //Prikaz framea u prozoru "Live camera"


		if (waitKey(30) == 27) //Prekid snimanja
		{
			cout << "esc key is pressed by user" << endl;
			break;
		}
	}
	dataset.close();
	return 0;
}