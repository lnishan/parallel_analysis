#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>
#include <ctime>
#include <omp.h>

#define SHOW_INFO false
#define SHOW_VIDEO false
#define OUTPUT_VIDEO true

using namespace std;
using namespace cv;

VideoWriter setOutput(const VideoCapture &input) {
	// Reference from
	// http://docs.opencv.org/2.4/doc/tutorials/highgui/video-write/video-write.html

	// Acquire input size
	Size S = Size((int) input.get(CV_CAP_PROP_FRAME_WIDTH),
				  (int) input.get(CV_CAP_PROP_FRAME_HEIGHT));

	 // Get Codec Type- Int form
	int ex = static_cast<int>(input.get(CV_CAP_PROP_FOURCC));

	VideoWriter output;
	output.open("outputVideo.avi", ex, input.get(CV_CAP_PROP_FPS), S, true);

    return output;
}

struct BGR {
	int b, g, r;
	BGR(int b,int g,int r)
	:b(b), g(g), r(r) {}
};

int threadNum;

void whiteBalance(Mat &img) {

	int rows = img.rows;
	int cols = img.cols;
	int picSz = rows * cols;

	int bSum=0, gSum=0, rSum=0;
	int avg[3], base;

	omp_set_num_threads(threadNum);
	#pragma omp parallel
	{
		#pragma omp for reduction(+:bSum,gSum,rSum)
		for(int i=0; i<rows; ++i)
			for(int j=0; j<cols; ++j) {
				bSum += img.at<Vec3b>(i,j)[0];
				gSum += img.at<Vec3b>(i,j)[1];
				rSum += img.at<Vec3b>(i,j)[2];
			}

		#pragma omp single
		{
		avg[0] = bSum / picSz;
		avg[1] = gSum / picSz;
		avg[2] = rSum / picSz;

		if( SHOW_INFO )
			printf("avg(b, g, r): %d %d %d\n",avg[0], avg[1], avg[2]);

		base = avg[1];
		}

		// let gAvg = bAvg = rAvg
		#pragma omp for nowait
		for(int i=0; i<rows; ++i)
			for(int j=0; j<cols; ++j) {
				img.at<Vec3b>(i,j)[0] = min(255, 
					(int)(base * img.at<Vec3b>(i,j)[0] / avg[0]));
				img.at<Vec3b>(i,j)[1] = min(255, 
					(int)(base * img.at<Vec3b>(i,j)[1] / avg[1]));
				img.at<Vec3b>(i,j)[2] = min(255,
					(int)(base * img.at<Vec3b>(i,j)[2] / avg[2]));
			}
	}
}

int main(int argc, const char** argv){
	if (CV_MAJOR_VERSION < 3) {
		puts("Advise you update to OpenCV3");
	}
	if( argc<2 ) {
		puts("Please specify input image path");
		return 0;
	}
	if( argc<3 ) {
		puts("Please specify thread num");
		return 0;
	}

	VideoCapture captureVideo(argv[1]);
	if( !captureVideo.isOpened() ) {
		puts("Fail to open video");
		return 0;
	}

	// Setup video output
	VideoWriter outputVideo;
	if( OUTPUT_VIDEO )
		outputVideo = setOutput(captureVideo);

	threadNum = atoi(argv[2]);
	if( SHOW_INFO )
		printf("threads: %d\n", threadNum);

	clock_t Coculate=0, Input=0, Output=0;
	clock_t Total = clock(), Last;

	Mat img;
	while( true ) {
		Last = clock();
		captureVideo >> img;
		if (img.empty()) break;
		Input += clock() - Last;

		Last = clock();
		whiteBalance(img);
		Coculate += clock() - Last;

		if( OUTPUT_VIDEO ) {
			Last = clock();
			outputVideo << img;
			Output += clock() - Last;
		}
	}

	Total = clock() - Total;

	printf("    Total: %fms (include time count)\n", 1.0*Total / (1.0*CLOCKS_PER_SEC / 1000.0));
	printf("    Input: %fms\n", 1.0*Input / (1.0*CLOCKS_PER_SEC / 1000.0));
	printf("   Output: %fms\n", 1.0*Output / (1.0*CLOCKS_PER_SEC / 1000.0));
	printf("Calculate: %fms\n", 1.0*Coculate / (1.0*CLOCKS_PER_SEC / 1000.0));

	return 0;
}
