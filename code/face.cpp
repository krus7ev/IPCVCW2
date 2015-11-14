/////////////////////////////////////////////////////////////////////////////
//
// COMS30121 - face.cpp
//
/////////////////////////////////////////////////////////////////////////////

// header inclusion
#include "/usr/local/opencv-2.4/include/opencv2/objdetect/objdetect.hpp"
#include "/usr/local/opencv-2.4/include/opencv2/opencv.hpp"
#include "/usr/local/opencv-2.4/include/opencv2/core/core.hpp"
#include "/usr/local/opencv-2.4/include/opencv2/highgui/highgui.hpp"
#include "/usr/local/opencv-2.4/include/opencv2/imgproc/imgproc.hpp"

#include <iostream>
#include <stdio.h>

using namespace std;
using namespace cv;

/** Function Headers */
void detectAndDisplay(Mat frame);
string splitFilename (string str);

/** Global variables */
String cascade_name = "data/frontalface.xml";
CascadeClassifier cascade;


/** @function main */
int main( int argc, const char** argv )
{
  // 1. Read Input Image and extract file name
	Mat frame = imread(argv[1], CV_LOAD_IMAGE_COLOR);
  string filename = splitFilename(argv[1]);

	// 2. Load the Strong Classifier in a structure called `Cascade'
	if( !cascade.load( cascade_name ) )
  { 
    printf("--(!)Error loading\n"); 
    return -1; 
  };

	// 3. Detect Faces and Display Result
	detectAndDisplay( frame );

	// 4. Save Result Image
	imwrite( "out/" + filename + "_detected.jpg", frame );

	return 0;
}

string splitFilename (string str)
{
  size_t dot = str.find_last_of(".");
  size_t fSlash = str.find_last_of("/\\");
  string name = str.erase(dot, string::npos);
  name = name.substr(fSlash+1);

  return name;  
}

/** @function detectAndDisplay */
void detectAndDisplay( Mat frame )
{
	std::vector<Rect> faces;
	Mat frame_gray;

	// 1. Prepare Image by turning it into Grayscale and normalising lighting
	cvtColor( frame, frame_gray, CV_BGR2GRAY );
	equalizeHist( frame_gray, frame_gray );

	// 2. Perform Viola-Jones Object Detection 
	cascade.detectMultiScale( frame_gray, faces, 1.1, 1, 0|CV_HAAR_SCALE_IMAGE, Size(50, 50), Size(500,500) );

  // 3. Print number of Faces found
	std::cout << faces.size() << std::endl;

  // 4. Draw box around faces found
	for( int i = 0; i < faces.size(); i++ )
	{
		rectangle(frame, Point(faces[i].x, faces[i].y), Point(faces[i].x + faces[i].width, faces[i].y + faces[i].height), Scalar( 0, 255, 0 ), 2);
	}

}

