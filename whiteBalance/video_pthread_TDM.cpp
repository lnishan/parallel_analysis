#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <vector>
#include <algorithm>
#include <ctime>
#include <thread>

#define SHOW_INFO false
#define OUTPUT_VIDEO true
#define TD_MAX_SIZE 500

using namespace std;
using namespace cv;

int threadNum;
int sz;
vector<Mat> imgs;

VideoWriter setOutput(const VideoCapture &input) {
	// Reference from
	// http://docs.opencv.org/2.4/doc/tutorials/highgui/video-write/video-write.html

	// Acquire input size
	Size S = Size((int)input.get(CV_CAP_PROP_FRAME_WIDTH),
		(int)input.get(CV_CAP_PROP_FRAME_HEIGHT));

	// Get Codec Type- Int form
	int ex = static_cast<int>(input.get(CV_CAP_PROP_FOURCC));

	VideoWriter output;
	output.open("outputVideo.avi", CV_FOURCC('P', 'I', 'M', '1'), input.get(CV_CAP_PROP_FPS), S, true);

	return output;
}

// Setup video output
void whiteBalance(int rank) {

	for (int id = rank; id<imgs.size() - 1; id += threadNum) {

		int rows = imgs[id].rows;
		int cols = imgs[id].cols;
		int picSz = rows * cols;

		if (imgs[id].isContinuous()) {
			cols *= rows;
			rows = 1;
		}

		int bSum = 0, gSum = 0, rSum = 0;
		double avg[3], base;

		for (int i = 0; i<rows; ++i) {
			Vec3b *p = imgs[id].ptr<Vec3b>(i);
			for (int j = 0; j<cols; ++j) {
				bSum += p[j][0];
				gSum += p[j][1];
				rSum += p[j][2];
			}
		}

		avg[0] = (double)bSum / picSz;
		avg[1] = (double)gSum / picSz;
		avg[2] = (double)rSum / picSz;

		if (SHOW_INFO)
			printf("avg(b, g, r): %f %f %f\n", avg[0], avg[1], avg[2]);

		base = avg[1];

		int tableB[256], tableG[256], tableR[256];
		for (int i = 0; i<256; ++i) {
			tableB[i] = min(255, (int)(base * i / avg[0]));
			tableG[i] = min(255, (int)(base * i / avg[1]));
			tableR[i] = min(255, (int)(base * i / avg[2]));
		}

		// let gAvg = bAvg = rAvg
		for (int i = 0; i<rows; ++i) {
			Vec3b *p = imgs[id].ptr<Vec3b>(i);
			for (int j = 0; j<cols; ++j) {
				p[j][0] = tableB[p[j][0]];
				p[j][1] = tableG[p[j][1]];
				p[j][2] = tableR[p[j][2]];
			}
		}
	}
}

int main(int argc, const char** argv){
	if (CV_MAJOR_VERSION < 3) {
		puts("Advise you update to OpenCV3");
	}
	if (argc<2) {
		puts("Please specify input image path");
		return 0;
	}
	if (argc<3) {
		puts("Please specify thread num");
		return 0;
	}

	VideoCapture captureVideo(argv[1]);
	if (!captureVideo.isOpened()) {
		puts("Fail to open video");
		return 0;
	}

	// Setup video output
	VideoWriter outputVideo;
	if (OUTPUT_VIDEO)
		outputVideo = setOutput(captureVideo);

	threadNum = atoi(argv[2]);
	if (SHOW_INFO)
		printf("threads: %d\n", threadNum);
	outputVideo.set(CV_CAP_PROP_BUFFERSIZE, 1);

	double Calculate = 0, Input = 0, Output = 0;
	double Total = getTickCount(), Last;

	while (true) {
		// input enough frames
		Last = getTickCount();
		for (int i = 0; i<TD_MAX_SIZE; ++i) {
			imgs.push_back(Mat());
			captureVideo >> imgs.back();
			if (imgs.back().empty()) break;
		}
		if (imgs[0].empty()) break;
		Input += getTickCount() - Last;

		// proc all got frames
		Last = getTickCount();
		vector<thread> threads;
		for (int i = 0; i<threadNum; ++i)
			threads.emplace_back(thread(whiteBalance, i));
		for (int i = 0; i<threadNum; ++i)
			threads[i].join();
		Calculate += getTickCount() - Last;

		if (OUTPUT_VIDEO) {
			Last = getTickCount();
			for (int i = 0; i<imgs.size() - 1; ++i)
				outputVideo << imgs[i];
			Output += getTickCount() - Last;
		}
		imgs.clear();
	}

	Total = getTickCount() - Total;

	printf("    Total: %.3fs (include time count)\n", Total / getTickFrequency());
	printf("    Input: %.3fs\n", Input / getTickFrequency());
	printf("   Output: %.3fs\n", Output / getTickFrequency());
	printf("Calculate: %.3fs\n", Calculate / getTickFrequency());

	return 0;
}

