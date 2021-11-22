//C:\Users\P1nkLemonade_01\Downloads\Resources\Resources\paper.jpg

#include <iostream>
#include <opencv2/opencv.hpp>
//#include <opencv2/imgcodecs.hpp>
//#include <opencv2/highgui.hpp>
//#include <opencv2/imgproc.hpp>
#include <vector>

using namespace cv;
using namespace std;

Mat img_orig, img_scan;
vector<Point> initialPoints, docsPoints;
float w = 420, h = 596;

Mat pre_processamento(Mat img) {
	Mat img_gray, img_canny, img_blur, img_dil, img_erode;
	cvtColor(img, img_gray, COLOR_BGR2GRAY);
	imshow("Gray", img_gray);
	GaussianBlur(img_gray, img_blur, Size(3, 3), 3, 0);
	imshow("Blur", img_blur);
	Canny(img_blur, img_canny, 25, 75);
	imshow("Canny", img_canny);

	Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
	dilate(img_canny, img_dil, kernel);
	//erode(img_dil, img_erode, kernel);
	return img_dil;
}

Mat scanner_processing(Mat img) {
	Mat img_gray2;
	cvtColor(img, img_gray2, COLOR_BGR2GRAY);
	imshow("adaptative threshold", img_gray2);
	adaptiveThreshold(img_gray2, img_scan, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 11, 2);//ADAPTIVE_THRESH_MEAN_C
	return img_scan;
}

vector<Point> getContours(Mat img) {
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;

	findContours(img, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
	//drawContours(img, contours, -1, Scalar(0, 0, 255), 2);
	vector<vector<Point>> conPoly(contours.size());
	vector<Rect> boundRect(contours.size());

	vector<Point> biggest;
	int maxArea = 0;

	for (int i = 0; i < contours.size(); i++) {
		int area = contourArea(contours[i]);
		//cout << area << endl;

		string objectType;

		if(area > 1000) {
			float peri = arcLength(contours[i], true);
			approxPolyDP(contours[i], conPoly[i], 0.02 * peri, true);

			if(area > maxArea && conPoly[i].size() == 4) {
				//drawContours(img_orig, conPoly, i, Scalar(0, 255, 0), 3);
				biggest = { conPoly[i][0], conPoly[i][1], conPoly[i][2], conPoly[i][3] };
				cout << biggest[i] << endl;
				maxArea = area;
			}

			//drawContours(img_orig, conPoly, i, Scalar(255, 0, 255), 2);
			//rectangle(img, boundRect[i].tl(), boundRect[i].br(), Scalar(0, 255, 0), 2)
		}
	}
	return biggest;
}

void drawPoints(vector<Point> points, Scalar color, Mat img) {
	for (int i = 0; i < points.size(); i++) {
		circle(img, points[i], 8, color, FILLED);
		putText(img, to_string(i), points[i], FONT_HERSHEY_PLAIN, 2, color, 2);
	}
}

vector<Point> reorder(vector<Point> points) {
	vector<Point> newPoints;
	vector<int> sumPoints, subPoints;

	for (int i = 0; i < 4; i++) {//4
		sumPoints.push_back(points[i].x + points[i].y);
		subPoints.push_back(points[i].x - points[i].y);
	}
	
	newPoints.push_back(points[min_element(sumPoints.begin(), sumPoints.end()) - sumPoints.begin()]); //0
	newPoints.push_back(points[max_element(subPoints.begin(), subPoints.end()) - subPoints.begin()]);
	newPoints.push_back(points[min_element(subPoints.begin(), subPoints.end()) - subPoints.begin()]);
	newPoints.push_back(points[max_element(sumPoints.begin(), sumPoints.end()) - sumPoints.begin()]); //3

	return newPoints;
}

Mat getWarp(Mat img, vector<Point> points, float w, float h) {
	Mat img_scanned;
	Point2f src[4] = { points[0], points[1], points[2], points[3] };
	Point2f dst[4] = { {0.0f, 0.0f}, {w, 0.0f}, {0.0f, h}, {w, h} };

	Mat matrix = getPerspectiveTransform(src, dst);
	warpPerspective(img, img_scanned, matrix, Point2i(w, h));

	return img_scanned;
}

Mat processing(Mat src) {
	//Mat src;
	//capture >> src;
	//src = imread(img);
	Mat img_pp, img_warp, img_crop, img_scanned;
	vector<Point> initialPoints, docsPoints;

	int cropVal = 5;

	//resize(src, src, Size(), 0.5, 0.5);

	img_pp = pre_processamento(src);

	initialPoints = getContours(img_pp);
	//drawPoints(initialPoints, Scalar(0, 0, 255), src);
	docsPoints = reorder(initialPoints);
	Mat src2 = src;
	//drawPoints(docsPoints, Scalar(0, 0, 255), src2);

	img_warp = getWarp(src, docsPoints, w, h);

	Rect roi(cropVal, cropVal, w - (2 * 3), h - (2 * 5));
	img_crop = img_warp(roi);
	resize(img_crop, img_crop, Size(), 0.5, 0.5);

	img_scanned = scanner_processing(img_crop);

	//imshow("Imagem enquadrada", src2);
	imshow("Imagem Preproccessada", img_pp);
	imshow("Imagem Cortada", img_crop);
	imshow("Imagem Escaneada", img_scanned);
	//imshow(janela_name, img_scanned);

	return img_scanned;
}

void captureWebcam(Mat frame) {
	VideoCapture cap(0);

	string janela_name = "Webcam escaneada";

	cap.set(3, 320);//640
	cap.set(4, 240);//480
	cap.set(10, 100);//brilho

	if (!cap.isOpened()) {
		cout << "Camera nao esta disponivel" << endl;
		return;
	}
	double dLargura = cap.get(CAP_PROP_FRAME_WIDTH);
	double dAltura = cap.get(CAP_PROP_FRAME_HEIGHT);
	cout << "Resolucao do video: " << dLargura << " x " << dAltura << endl;

	while (true) {
		cap >> frame;
		//cap.read(img_orig);
		bool stateCam = cap.read(frame);
		if (stateCam == false) {
			cout << "Webcam foi desconectada" << endl;
			cin.get();
			break;
		}

		char tecla_print = waitKey(1);
		if (tecla_print == 'p') {
			imshow("Imagem print", frame);
			img_scan = processing(frame);
		}
		//namedWindow(janela_name, WINDOW_NORMAL);

		//resize(img_orig, img_orig, Size(), 1, 1);

		//img_pp = pre_processamento(img_orig);

		//initialPoints = getContours(img_pp);

		//for (int j = 0; j < 4; j++) {
		//	cout << initialPoints[j] << endl;
		//}

		//drawPoints(initialPoints, Scalar(0, 0, 255)); //pontos desordenados
		//docsPoints = reorder(initialPoints);
		//drawPoints(docsPoints, Scalar(0, 0, 255));
		
		//img_warp = getWarp(img_orig, docsPoints, w, h);

		//int cropVal = 5;
		//Rect roi(cropVal, cropVal, w - (2 * 5), h - (2 * 5));
		//img_crop = img_warp(roi);

		//img_scanned = pre_processamento2(img_crop);
		//imshow("Imagem pura", img_orig);
		imshow("Imagem real time", frame);
		//imshow("Imagem warp", img_scanned);
		//imshow("Imagem crop", img_crop);
		//imshow(janela_name, img_scanned);


		char tecla_press = waitKey(1)/* && 0xff*/;
		if (tecla_press == 'q')
			break;
	}

	cap.release();
	destroyAllWindows();
}

int main() {
	Mat frame;
	string imgPath = "C:\\Users\\P1nkLemonade_01\\Downloads\\Resources\\Resources\\paper.jpg";//C:\\Users\\P1nkLemonade_01\\Downloads\\Resources\\Resources\\declaracao_de_vinculo.png
	img_orig = imread(imgPath);

	//img_scan = processing(img_orig);

	//Mat img_pp, img_warp, img_crop, img_scanned;
	//vector<Point> initialPoints, docsPoints;

	//int cropVal = 5;

	//resize(img_orig, img_orig, Size(), 0.5, 0.5);

	//img_pp = pre_processamento(img_orig);

	//initialPoints = getContours(img_pp);
	////drawPoints(initialPoints, Scalar(0, 0, 255), img_orig);
	//docsPoints = reorder(initialPoints);
	////drawPoints(docsPoints, Scalar(0, 0, 255), img_orig);

	//img_warp = getWarp(img_orig, docsPoints, w, h);

	//Rect roi(cropVal, cropVal, w - (2 * 5), h - (2 * 5));
	//img_crop = img_warp(roi);

	//img_scanned = scanner_processing(img_crop);

	//imshow("Imagem Pura", img_orig);
	//imshow("Imagem Preproccessada", img_pp);
	//imshow("Imagem Cortada", img_crop);
	//imshow("Imagem Escaneada", img_scanned);
	//imshow(janela_name, img_scanned);


	captureWebcam(frame);
	//waitKey();
	return 0;
}