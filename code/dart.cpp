///////////////////////////////////////////////////////////////////////////////
//
// COMS30121 - dart.cpp
// by Roland Baranyi and Kristian Krastev(rb12809, kk12742)
//
///////////////////////////////////////////////////////////////////////////////

//g++ dart.cpp /usr/local/opencv-2.4/lib/libopencv_core.so.2.4 /usr/local/lib/libopencv_highgui.so.2.4 /usr/local/lib/libopencv_imgproc.so.2.4 /usr/local/lib/libopencv_objdetect.so.2.4

// on Snowy, execute this command to compile:
// g++ dart.cpp  /usr/local/opencv-2.4/lib/libopencv_core.so.2.4 
//               /usr/local/opencv-2.4/lib/libopencv_highgui.so.2.4 
//               /usr/local/opencv-2.4/lib/libopencv_imgproc.so.2.4 
//               /usr/local/opencv-2.4/lib/libopencv_objdetect.so.2.4

// and execute this command to run :
// ./a.out images/tests/dart#.jpg, where # = 0 to 15

// Header inclusion
#include "/usr/local/include/opencv2/objdetect/objdetect.hpp"
#include "/usr/local/include/opencv2/opencv.hpp"
#include "/usr/local/include/opencv2/core/core.hpp"
#include "/usr/local/include/opencv2/highgui/highgui.hpp"
#include "/usr/local/include/opencv2/imgproc/imgproc.hpp"

#include <iostream>
#include <stdio.h>

using namespace std;
using namespace cv;

/** Function prototypes */
void detectAndDisplay(Mat frame_in, Mat &frame_out, vector<Rect> &dartboards);

void gradient(Mat &inImage, Mat &outGradMag, Mat &outGradMag_uchar, 
  Mat &outGradDir, Mat &outGradDir_uchar, int Ts);

void convolve(Mat &input, Mat &kernel, Mat &output, double *minSum, 
  double *maxSum);

void normalise(Mat &convFloat, Mat &output, double *minSum, double *maxSum);

void threshold(Mat &input, int Ts);

void threshold2 (Mat &input, int Ts);

void threshold3 (Mat &input, int TsL, int TsH);

void gradMag(Mat &inDx, Mat &inDy, Mat &outMag, double *min, double *max);

void gradDir(Mat &inDx, Mat &inDy, Mat &outDir, double *min, double *max);

void houghCircle(Mat &outDyDx_uchar, Mat &outgradDir, Mat &houghVis, 
  int minRad, int maxRad);

void houghLineXY(Mat &gradMag, Mat &gradDir, Mat &houghVis);

void houghCombine(Mat &hCircle, Mat &hLines, Mat &hCombined);

void houghLineDA(Mat &outGradMag_uchar, Mat &outGradDir);

void weighBoxes(vector<Rect> &dartboards, int votes[], Mat &hough_line, 
  Mat &hough_circ, vector<Point3i> &boxMax);

bool boxOverlap(Rect a, Rect b);

void filterBoxes(vector<Rect> &dartboards, int votes[], int Ts, Mat &frame, 
  string filename, vector<Point3i> &boxMax);

string splitFilename(string str);

// ----------------------------------------------------------------------------

/** Global variables */
String cascade_name = "data/dartcascade.xml";
CascadeClassifier cascade;
string filename;

// ----------------------------------------------------------------------------

/** MAIN Procedure */
int main(int argc, const char** argv)
{
  // Read Input Image and extract file name
	Mat frame = imread(argv[1], CV_LOAD_IMAGE_COLOR);
  filename = splitFilename(argv[1]);
  Mat frame_vj;

	// Load the Strong Classifier in a structure called ;ascade'
	if(!cascade.load(cascade_name))
  { 
    printf("--(!)Error loading\n"); 
    return -1; 
  };

  // Detect dartboards using Viola Jones and draw bounding boxes arround them
  vector<Rect> dartboards;
	detectAndDisplay(frame, frame_vj, dartboards);
 
  Mat grad_mag, grad_mag_uc;
  Mat grad_dir, grad_dir_uc;
  int threshGradMag = 45;
  gradient(frame, grad_mag, grad_mag_uc, grad_dir, grad_dir_uc, threshGradMag);

  int minRad = 3;
  int maxRad =(min(frame.cols, frame.rows)) / 2;
    
  Mat h_circ_uc;
  cout << "\nGenerating Hough Circle Transform..." << endl;
  houghCircle(grad_mag_uc, grad_dir, h_circ_uc, minRad, maxRad);
  threshold2(h_circ_uc, 60);
  imwrite("houghCircsThresh.png", h_circ_uc);
  
  //Mat h_circ_blur;
  //bilateralFilter(h_circ_uc, h_circ_blur, 9, 200, 200);
  //imwrite("bluredHoughCirc.png", h_circ_blur); 
  
  Mat h_line_uc;
  cout << "Generating Hough Line Transform..." << endl;
  //houghLineDA(grad_mag_uc, grad_dir);
  houghLineXY(grad_mag_uc, grad_dir, h_line_uc);
  threshold3(h_line_uc, 35, 88);
  imwrite("houghLinesThresh.png", h_line_uc);
  
  /*double kernelHP[3][3]= {
    {-1, -1, -1},
    {-1, 16, -1},
    {-1, -1, -1}};
    
  Mat highPass(3,3, CV_64F, kernelHP);
  Mat linesHP_float;
  Mat linesHP_uchar;
  double min = 30000.0; double max = 0.0;

  convolve(h_line_uc, highPass, linesHP_float, &min, &max);
  normalise(linesHP_float, linesHP_uchar, &min, &max); 
  imwrite("linesHHP.png", linesHP_uchar); 
   */

  //Mat h_comb_uc;
  //cout << "Generating Hough Combined Transform..." << endl;
  //houghCombine(h_circ_uc, h_line_uc, h_comb_uc);

  int Ts = 158;
  cout << "\nThreshold: " << Ts << "\n" << endl;
  int votes[dartboards.size()];
  vector<Point3i> boxMax;
  weighBoxes(dartboards, votes, h_line_uc, h_circ_uc, boxMax);
  filterBoxes(dartboards, votes, Ts, frame, filename, boxMax);
  
	return 0;
}

// ----------------------------------------------------------------------------

/** @function filterBoxes */
void filterBoxes(vector<Rect> &dartboards, int votes[], int Ts, Mat &frame, 
  string filename, vector<Point3i> &boxMax)
{
  vector<Rect> goodDarts;
  vector<Point3i> goodBoxMax;
  int count = 0;
  for(int i = 0; i < dartboards.size(); i++) 
  {
    if(votes[i] > Ts)
    {
      goodDarts.push_back(dartboards[i]);
      goodBoxMax.push_back(boxMax[i]);  
      cout<<"voteofbox: "<<votes[i]<<endl;  
    }
  }
 
  for(int i = 0; i < goodDarts.size(); i++) 
  {
    for( int j = i+1; j < goodDarts.size(); j++)
    {
      if( boxOverlap(goodDarts[i], goodDarts[j]) )
      {
        if (goodBoxMax[i].z == goodBoxMax[j].z)
        {
          int iCentX = goodDarts[i].x + goodDarts[i].width/2;
          int iCentY = goodDarts[i].y + goodDarts[i].height/2;
          int jCentX = goodDarts[j].x + goodDarts[j].width/2;
          int jCentY = goodDarts[j].y + goodDarts[j].height/2;
                             
          int distI = sqrt((iCentX - goodBoxMax[i].x)*(iCentX-goodBoxMax[i].x)
                         + (iCentY - goodBoxMax[i].y)*(iCentY-goodBoxMax[i].y));
          
          int distJ = sqrt((jCentX - goodBoxMax[j].x)*(jCentX-goodBoxMax[j].x)
                         + (jCentY - goodBoxMax[j].y)*(jCentY-goodBoxMax[j].y));
                         
          
          if (distI > distJ)
          {
            goodDarts[i] = goodDarts[j];
          }
          goodDarts.erase(goodDarts.begin() + j);
          j--;              
        }
        else
        {
          if (goodBoxMax[i].z < goodBoxMax[j].z )
          {
            goodDarts[i] = goodDarts[j];
          }
          goodDarts.erase(goodDarts.begin() + j);
          j--;       
        }
      
      }
    }
  }
  
  for(int i = 0; i < goodDarts.size(); i++)
  {
    rectangle(frame, Point(goodDarts[i].x, goodDarts[i].y), 
      Point(goodDarts[i].x + goodDarts[i].width, 
      goodDarts[i].y + goodDarts[i].height), Scalar(0, 255, 0), 2); 
  }
  imwrite("out/" + filename + "_detected.jpg", frame);
  cout << "\nFiltered count: " << goodDarts.size() << "\n" << endl;

}

// ----------------------------------------------------------------------------

/** @function boxOverlap */
bool boxOverlap(Rect a, Rect b) 
{
  return (abs(a.x - b.x) * 2 < (a.width + b.width)) &&
         (abs(a.y - b.y) * 2 < (a.height + b.height));
}

// ----------------------------------------------------------------------------

/** @function weighBoxes */
void weighBoxes(vector<Rect> &dartboards, int votes[], Mat &hough_line, 
  Mat &hough_circ, vector<Point3i> &boxMax)
{
  int count = 0;
  int maxCirc = 0;
  vector<Point> clusterMax;
  int xCMu = 0;
  int yCMu = 0;
  
  int maxLine = 0;
  vector<Point> clusterMax2;
  int xLMu = -1;
  int yLMu = -1;
    
  int weightCirc = 1;
  int weightLine = 3;
  int weightDist = 2;
  int weightSum = weightCirc + weightLine + weightDist;
  int xC, yC = -1;
  int xL, yL = -1;
  int pixNum = 0;
  int dist = 0;
  int weightedDist = 0; 
  
	for(int i = 0; i < dartboards.size(); i++)
	{
    for(int y = dartboards[i].y +(dartboards[i].height/4); y < dartboards[i].y 
      +(dartboards[i].height * 3/4); y++)
    {
      for(int x = dartboards[i].x +(dartboards[i].width/4); x < dartboards[i].x
        +(dartboards[i].width * 3/4); x++)
      {
        if(maxLine < hough_line.at<uchar>(y,x) )
        {
          maxLine = hough_line.at<uchar>(y,x);
          xL = x;
          yL = y;
        }  
        if(maxCirc < hough_circ.at<uchar>(y,x) )
        {
          maxCirc = hough_circ.at<uchar>(y,x);
          xC = x;
          yC = y;
        }
      }
    }
    
    boxMax.push_back(Point3i(xL, yL, maxLine));
    
    if ((maxLine>0) && (maxCirc>0))
    {    
      cout<<endl<<"First Max Coordinates: "<<xC<<" , "<<yC<<endl;
      
      for(int y = dartboards[i].y +(dartboards[i].height/4); 
        y < dartboards[i].y + (dartboards[i].height * 3/4); y++)
      {
        for(int x = dartboards[i].x +(dartboards[i].width/4); 
          x < dartboards[i].x + (dartboards[i].width * 3/4); x++)
        {  
          if(hough_circ.at<uchar>(y,x) > (maxCirc-3) )
          {
            clusterMax.push_back(Point(y,x));
            xCMu += x;
            yCMu += y; 
            //cout<<endl;cout<<endl;
            //cout <<"Cluster coordinates: "<<clusterMax.back().x<<" , "<<clusterMax.back().y<<endl;
            //cout<<endl<<"Accumulated coordinates: "<<xC<<" , "<<yC<<endl;
            //cout<<endl;cout<<endl;
          }
          
          if(hough_line.at<uchar>(y,x) == maxLine )
          {
            clusterMax2.push_back(Point(y,x));
            xLMu += x;
            yLMu += y; 
          }
        }
      }
      // get mean of cluster
      xCMu = xCMu/clusterMax.size();
      yCMu = yCMu/clusterMax.size();
      
      xLMu = xLMu/clusterMax2.size();
      yLMu = yLMu/clusterMax2.size();
      
      cout<<endl<<"Mean Coords Hough Circles: "<<xCMu<<" , "<<yCMu<<endl;
      cout<<"number of points in cluster HC: "<<clusterMax.size()<<endl;
      cout<<endl<<"Mean Coords Hough Lines: "<<xLMu<<" , "<<yLMu<<endl;
      cout<<"number of points in cluster HL: "<<clusterMax.size()<<endl;
      
      //compute distance between maximums in hough spaces
      dist = sqrt((xLMu-xCMu)*(xLMu-xCMu) + (yLMu-yCMu)*(yLMu-yCMu)+1);
      cout<<"xLMu yLMu xCMu yCMu:"<<xLMu<<" "<<yLMu<<" "<<xCMu<<" "<<yCMu<<endl;
      
      weightedDist = (int)(pow(255.0, 1.779)*( 1.0/(30.0*sqrt(2.0*M_PI)) * 
        exp(-((dist-1)*(dist-1))/(2.0*900.0)) ));
      cout << "weightedDist = "<< weightedDist <<endl;
      cout<<endl;
      //compute vote based on the 3 weighted features
      votes[i] = ( weightLine*maxLine + weightCirc*maxCirc + 
        weightDist*weightedDist )/weightSum;
      
      //reinitialize variables for next iteration through rect vect
      
    }
    else
    {
      votes[i] = 0;
    }
    
    maxCirc = 0;
    maxLine = 0;
    xC = yC = xL = yL = -1;
    xCMu = -1;
    yCMu = -1;
    xLMu = -1;
    yLMu = -1;
    dist = 0;
    weightedDist = 0;
    clusterMax.clear();
    clusterMax2.clear();
  }
}

// ----------------------------------------------------------------------------

/** @function detectAndDisplay */
void detectAndDisplay(Mat frame_in, Mat &frame_out, vector<Rect> &dartboards)
{
	//vector<Rect> dartboards;
	Mat frame_gray;
  frame_out.create(frame_in.size(), frame_in.type());

	// Prepare Image by converting to grayscale turning normalising lighting
  cvtColor(frame_in, frame_gray, CV_BGR2GRAY);
	equalizeHist(frame_gray, frame_gray);

	// Perform Viola-Jones Object Detection 
	cascade.detectMultiScale(frame_gray, dartboards, 1.1, 1, 
    0|CV_HAAR_SCALE_IMAGE, Size(50, 50), Size(500,500));

  // Print number of dartboards found
	cout << "\nNumber of dartboards found using Viola Jones: " 
	  << dartboards.size() << endl;
}

// ----------------------------------------------------------------------------

/** @function gradient */
void gradient(Mat &inImage, Mat &outGradMag, Mat &outGradMag_uchar, 
  Mat &outGradDir, Mat &outGradDir_uchar, int Ts)
{
  Mat grayImage;
  cvtColor(inImage, grayImage, CV_BGR2GRAY);

  // INITIALISE SOBEL FILTER KERNELS FOR DERIVATIVES ACROSS X AND Y DIMENSIONS
  double kernelX[3][3]= {
    {-1,  0,  1},
    {-2,  0,  2},
    {-1,  0,  1}};
    
  double kernelY[3][3]= {
    { 1,  2,  1},
    { 0,  0,  0},
    {-1, -2, -1}};
    
  Mat sobelDx(3,3, CV_64F, kernelX);
  Mat sobelDy(3,3, CV_64F, kernelY);  
 
  double minVal = 30000.0;
  double maxVal = 0.0;

  // Compute derivative with respect to X by convolving with  
  // a Sobel vertical edge kernel
  Mat outDfDx_float;
  Mat outDfDx_uchar;
  
  convolve(grayImage, sobelDx, outDfDx_float, &minVal, &maxVal);
  normalise(outDfDx_float, outDfDx_uchar, &minVal, &maxVal); 
  imwrite("DfDx.png", outDfDx_uchar); 

  // Compute derivative with respect to Y by convolving with 
  // a Sobel horizontal edge kernel
  Mat outDfDy_float;
  Mat outDfDy_uchar;
  
  convolve(grayImage, sobelDy, outDfDy_float, &minVal, &maxVal);
  normalise(outDfDy_float, outDfDy_uchar, &minVal, &maxVal); 
  imwrite("DfDy.png", outDfDy_uchar);

  // Compute derivative with respect to(X,Y) - gradient magnitude
  gradMag(outDfDx_float, outDfDy_float, outGradMag, &minVal, &maxVal);
  normalise(outGradMag, outGradMag_uchar, &minVal, &maxVal);   
  threshold(outGradMag_uchar, Ts);
  imwrite("gradMag.png", outGradMag_uchar);

  // Compute gradient direction
  gradDir(outDfDx_float, outDfDy_float, outGradDir, &minVal, &maxVal);
  normalise(outGradDir, outGradDir_uchar, &minVal, &maxVal);
  imwrite("gradDir.png", outGradDir_uchar);
}

// ----------------------------------------------------------------------------

// Applies and visualises a 2D Circle Hough Transform
/** @function houghCircle */
void houghCircle(Mat &outDyDx_uchar, Mat &outGradDir, Mat &houghVis, 
  int minRad, int maxRad) //votes' threshold parameter?
{
  // Intiialize parameter sace dimensions
  int rD = maxRad - minRad;
  int yD = outDyDx_uchar.rows;
  int xD = outDyDx_uchar.cols;
  
  // Initialize parameter space dynamically on dimension variables 
  int ***houghSpace;
  houghSpace = new int**[yD];

  for(int y0 = 0; y0 < yD; y0++)
  {	
    houghSpace[y0] = new int*[xD];
    
    for(int x0 = 0; x0 < xD; x0++)
    {
      houghSpace[y0][x0] = new int[rD];
      
      for(int r = 0; r < rD; r++)
      {
        houghSpace[y0][x0][r] = 0;
      }
    }
  }
  
  cout << "Hough Circle array initialised." << endl;

  // Go through each pixel(y,x) in the thresholed gradient magnitude image, and 
  // consider it only if it is white(255), and
  // consider the pixels(y0,x0) in the image on the line of its gradient, and
  // if the distance(r) to such a pixel is within the range of radiuses, then
  // increment the votes for circle defined by(y0,x0,r) in the Hough Space

  #pragma omp parallel for
  for(int y = 0; y < yD; y++)
  {	
    for(int x = 0; x < xD; x++)
    {
      if(outDyDx_uchar.at<uchar>(y,x) == 255)
      {
        for(int x0 = 0; x0 < xD; x0++)
        {      
          double tanGrad = tan(outGradDir.at<double>(y,x));
          int y0 =(int)(tanGrad*(x0-x) + y); //make some magic

          if(y0 > 0 && y0 < yD)
          {
            int r =(int)(sqrt((y0-y)*(y0-y) +(x0-x)*(x0-x)));
            
            if(r >= minRad && r <= maxRad)
            {
              houghSpace[y0][x0][r-minRad] += 1;
            }
          }
        }
      }
    }
  }
  
  cout << "Hough Circle array populated." << endl;
  
  Mat hMat = Mat::zeros(outDyDx_uchar.size(), CV_16U); 
  int maxx = 0; 
   
  for(int y0 = 0; y0 < yD; y0++)
  {
    for(int x0 = 0; x0 < xD; x0++)
    {
      for(int r = 0; r < rD; r++)
      {
        hMat.at<ushort>(y0,x0) += houghSpace[y0][x0][r];
        
        if(hMat.at<ushort>(y0,x0) > maxx)
        {
          maxx = hMat.at<ushort>(y0,x0);   
        }
      }
    }
  }

  houghVis.create(outDyDx_uchar.size(), outDyDx_uchar.type());
  #pragma omp parallel for
  for(int y0 = 0; y0 < yD; y0++)
  {
    for(int x0 = 0; x0 < xD; x0++)
    {
      houghVis.at<uchar>(y0,x0) = hMat.at<ushort>(y0,x0) * 255 / maxx;
    }
  }
  
  imwrite("houghCircs.png", houghVis);  
}

// ----------------------------------------------------------------------------

//Applies and visualises a custom 2D Line Hough Space mapped to the image plane
/** @function houghLineXY */
void houghLineXY(Mat &gradMag, Mat &gradDir, Mat &houghVis)

{
  // Initialize parameter space
  Mat hLines = Mat::zeros(gradMag.size(), CV_16U);
  Mat lastVoteDir = Mat::zeros(gradDir.size(), CV_64F);

  cout << "Hough Line array initialised." << endl;

  int yD = hLines.rows;
  int xD = hLines.cols;
  int max = 0;
  
  for(int y = 15; y < yD -15; y++)
  {	
	  for(int x = 15; x < xD -15; x++)
	  {
      if(gradMag.at<uchar>(y,x) == 255)
      {
        for(int x0 = x-15; x0 < x+15; x0++)
        { 
          double Dir1 = gradDir.at<double>(y,x) - M_PI*0.5;        
          
          //Derrive y from x for the line equation (using tangens).
          double tanDir = tan(Dir1);
          int y0 =(int)(tanDir*(x0-x) + y);
          
          //Shift the Zero point on the trigonometric circle from the vertical
          //to the horizontal axis.
          double Dir = fmod(Dir1 + 1.5*M_PI, 2*M_PI);
          
          if(y0 > 0 && y0 < yD)
          {
            //Reward 40 times more voting points if last voting point's line
            //angle was at least 4 degrees away from this voting point's line
            //angle. This exploits let-to-right looping through rows. Angles
            //constants are in radians in the constraints below.
            if ( ( (lastVoteDir.at<float>(y0,x0) > Dir + 0.069) 
                && (lastVoteDir.at<float>(y0,x0) < Dir + 3.072) ) 
              || ( (lastVoteDir.at<float>(y0,x0) < Dir - 0.069) 
                && (lastVoteDir.at<float>(y0,x0) > Dir - 3.072) ) )
            {
                hLines.at<ushort>(y0,x0) += 40;            
            }
            else
            {
              hLines.at<ushort>(y0,x0)++;
            }
            
            lastVoteDir.at<float>(y0,x0) = Dir;
            
            if(hLines.at<ushort>(y0,x0) > max)
            {
              max = hLines.at<ushort>(y0,x0);
            }
          }
        }
      }
    }
  }
  
  cout << "Hough Line array populated." << endl;
  
  houghVis.create(hLines.size(), CV_8U); 
  #pragma omp parallel for
  for(int y0 = 0; y0 < yD; y0++)
  {
    for(int x0 = 0; x0 < xD; x0++)
    {
      houghVis.at<uchar>(y0,x0) = hLines.at<ushort>(y0,x0) * 255 / max;
    }
  }
  
  imwrite("hLines.png", houghVis);  
}

// ----------------------------------------------------------------------------

/** @function houghCombine */
void houghCombine(Mat &hCircle, Mat &hLines, Mat &hCombined)
{
  Mat hMat(hLines.size(), CV_16U);
  
  cout << "Hough Combined array initialised." << endl;

  int yD = hLines.rows;
  int xD = hLines.cols;
  int max = 0; 
    
  for(int y0 = 0; y0 < yD; y0++)
  {
    for(int x0 = 0; x0 < xD; x0++)
    {
      hMat.at<ushort>(y0,x0) = hLines.at<uchar>(y0,x0) 
        + hCircle.at<uchar>(y0,x0);
        
      if(hMat.at<ushort>(y0,x0) > max)
      {
        max = hMat.at<ushort>(y0,x0);
      }
    }
  }
  
  cout << "Hough Combined array populated." << endl;
  
  hCombined.create(hLines.size(), CV_8U);
  #pragma omp parallel for
  for(int y0 = 0; y0 < yD; y0++)
  {
    for(int x0 = 0; x0 < xD; x0++)
    {
      hCombined.at<uchar>(y0,x0) = hMat.at<ushort>(y0,x0)*255/max;
    }
  }

  imwrite("hCombined.png", hCombined);  
}

// ----------------------------------------------------------------------------

//Applies and visualises a 2D Distance-Angle Line Hough Transform
/** @function houghLineDA */
void houghLineDA(Mat &gradMag, Mat &gradDir)
{
  // Intialise parameter space dimensions
  int roD =(int)sqrt(gradDir.rows*gradDir.rows + gradDir.cols*gradDir.cols); 
  int thetaD = 181; //theta is between 0 - 180 degrees

  // Initialize parameter space dynamically on dimension variables
  int **houghSpace = new int* [roD];
  
  for(int ro=0; ro < roD; ro++)
  {
    houghSpace[ro] = new int[thetaD];
    
    for(int th = 0; th < thetaD; th++)
    {
      houghSpace[ro][th]= 0;
    }
  }

  cout << "Hough Line array initialised." << endl;
  //int max = 0;
  double dTheta = 5; //delta tolerance around the gradient angle
  int dirDeg; //to store the gradient direction in degrees
  
  for(int y = 0; y < gradMag.rows; y++)
	{	
	  for(int x = 0; x < gradMag.cols; x++)
	  {
      if(gradMag.at<uchar>(y,x) == 255)
      {
        //convert gradient direction to degrees
        dirDeg =(int)((gradDir.at<double>(y,x))*180/ M_PI); 
        if(dirDeg < 0) dirDeg += 180;

        int dTheta = 8;
        
        // take care of corner cases on theta ranges below
        if(dirDeg < dTheta)
        {
          dirDeg = dTheta; 
        }
        
        if(dirDeg > (thetaD - 1 - dTheta))
        {
          dirDeg = thetaD - 1 - dTheta;
        }

        for(int theta = dirDeg - dTheta; theta <= dirDeg + dTheta; theta++)
        //for(int theta = 0; theta < thetaD; theta++)
        {
          for(int ro = 0; ro < roD; ro++)
          {
            if(ro ==(int)(x*cos(theta*M_PI/180) + y*sin(theta*M_PI/180)))
            {
              houghSpace[ro][theta]++;
              //if(houghSpace[ro][theta] > max)
              //{
              //  max = houghSpace[ro][theta];  
              //}               
            }
          }
        }
      }
    }
  }
  
  cout << "Hough Line array populated." << endl;
  
  Mat houghVis;
  houghVis.create(roD, thetaD, CV_8U);
  
  for(int y = 0; y < roD ; y++)
  {
    for(int x = 0; x < thetaD; x++)
    {
      houghVis.at<uchar>(y,x) = houghSpace[y][x];//*255/max;
      
      //if(houghSpace[y][x] > 4)
      //{
      //  cout <<"score("<<y<<", "<<x<<")= "<< houghSpace[y][x] << endl;   
      //}    
    }
  }
  
  imwrite("houghLines.png", houghVis);
}

// ----------------------------------------------------------------------------

/** @function convolve */
void convolve(Mat &input, Mat &kernel, Mat &output, double *minSum, 
  double *maxSum)
{
	// intialise the output using the input
	output.create(input.size(), CV_64F);

	// we need to create a padded version of the input
	// or there will be border effects
	int kernelRadiusY =(kernel.size[0] - 1) / 2;
	int kernelRadiusX =(kernel.size[1] - 1) / 2;

	Mat paddedInput;
	copyMakeBorder(input, paddedInput, 
		kernelRadiusY, kernelRadiusY, kernelRadiusX, kernelRadiusX,
		BORDER_REPLICATE);

  double convFloat[input.rows][input.cols];
  
	// now we can do the convoltion
	#pragma omp parallel for
	for(int y = 0; y < input.rows; y++)
	{	
		for(int x = 0; x < input.cols; x++)
		{
			double sum = 0;
			
			for(int i = -kernelRadiusY; i <= kernelRadiusY; i++)
			{
				for(int j = -kernelRadiusX; j <= kernelRadiusX; j++)
				{
					int kernely = i + kernelRadiusY;
					int kernelx = j + kernelRadiusX;

          int imagey = y - i + kernelRadiusY;
					int imagex = x - j + kernelRadiusX;

					// get the values from the padded image and the kernel
					double imageval =(double)paddedInput.at<uchar>(imagey, imagex);
					double kernalval = kernel.at<double>(kernely, kernelx);

					// do the multiplication
					sum += imageval * kernalval;							
				}
			}
			
			// set the output value as the sum of the convolution
      if(sum < *minSum)
      {
          *minSum = sum;
      }
      
      if(sum > *maxSum)
      {

          *maxSum = sum;
      }

     output.at<double>(y,x) = sum;  			
		}
	}
}

// ----------------------------------------------------------------------------

/** @function normalise */
void normalise(Mat &convFloat, Mat &output, double *minSum, double *maxSum)
{
  output.create(convFloat.size(), CV_8U);    

  double range = *maxSum - *minSum+1;
  double mMax = -1, mMin = 3000;

  for(int y = 0; y < output.rows; y++)
  {	
	  for(int x = 0; x < output.cols; x++)
	  {
      double temp =(convFloat.at<double>(y,x) - *minSum) * 255.0 / range;
      
      if(temp > mMax)
      {
          mMax = temp;
      }
      
      if(temp < mMin)
      {
          mMin = temp;
      }

      output.at<uchar>(y, x) =(uchar)temp;
    }
  }
  //cout<<"\n-- "<<mMin<<' '<<mMax<<endl; 
}

// ----------------------------------------------------------------------------

/** @function gradMag */
void gradMag(Mat &inDx, Mat &inDy, Mat &outMag, double *min, double *max)
{
  *min = 30000.0;
  *max = 0.0;

  outMag.create(inDx.size(), inDx.type());
 
  for(int y = 0; y < inDx.rows; y++)
  {	
	  for(int x = 0; x < inDx.cols; x++)
	  {
      double pDx = inDx.at<double>(y,x);
      double pDy = inDy.at<double>(y,x);
      double val = sqrt(pDx*pDx + pDy*pDy);

      if(val < *min)
      {
          *min = val;
      }
      
      if(val > *max)
      {

          *max = val;
      }
      
      outMag.at<double>(y,x) = val;

    }
  }
}

// ----------------------------------------------------------------------------

/** @function threshold */
void threshold(Mat &input, int Ts)
{
  for(int y = 0; y < input.rows; y++)
  {	
    for(int x = 0; x < input.cols; x++)
    {
      uchar val = input.at<uchar>(y,x); 
         
      if(val > Ts)
      {
         input.at<uchar>(y,x) = 255;
      }
      
      else
      { 
         input.at<uchar>(y,x) = 0;
      }    
    }
  }
}

// ----------------------------------------------------------------------------


/** @function threshold */
void threshold2 (Mat &input, int Ts)
{
  for(int y = 0; y < input.rows; y++)
  {	
    for(int x = 0; x < input.cols; x++)
    {
      if(input.at<uchar>(y,x) < Ts)
      {
         input.at<uchar>(y,x) = 0;
      }    
    }
  }
}

// ----------------------------------------------------------------------------

/** @function threshold */
void threshold3 (Mat &input, int TsL, int TsH)
{
  for(int y = 0; y < input.rows; y++)
  {	
    for(int x = 0; x < input.cols; x++)
    {
      if(input.at<uchar>(y,x) < TsL)
      {
         input.at<uchar>(y,x) = 0;
      } 
      if(input.at<uchar>(y,x) > TsH)
      {
         if(input.at<uchar>(y,x) < 215)
            input.at<uchar>(y,x) += 40;
      }   
    }
  }
}

// ----------------------------------------------------------------------------

/** @function gradDir */
void gradDir(Mat &inDx, Mat &inDy, Mat &outDir, double *min, double *max)
{
  *min = 30000.0;
  *max = 0.0;

  outDir.create(inDx.size(), CV_64F);

  for(int y = 0; y < inDx.rows; y++)
  {	
    for(int x = 0; x < inDx.cols; x++)
    {
      double pDx = inDx.at<double>(y,x);
      double pDy = inDy.at<double>(y,x);

      double val = atan2(pDx, pDy) + 1.5*M_PI; //Fi+pi/2 in radians

      if(val < *min)
      {
        *min = val;
      }
      
      if(val > *max)
      {
        *max = val;
      }

      outDir.at<double>(y,x) = val;
    }
  }
}

// ----------------------------------------------------------------------------

/** @function splitFilename */
string splitFilename(string str)
{
  size_t dot = str.find_last_of(".");
  size_t fSlash = str.find_last_of("/\\");
  string name = str.erase(dot, string::npos);
  name = name.substr(fSlash+1);

  return name;  
}

// ----------------------------------------------------------------------------

