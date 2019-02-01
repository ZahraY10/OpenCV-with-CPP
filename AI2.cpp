#include <iostream>
#include <vector>
#include <stdio.h>
#include "fstream"
#include "iostream"
#include <algorithm>
#include <iterator>
#include <random>
#include <pthread.h>
#include "opencv2/opencv.hpp"
using namespace std;
using namespace cv;
//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
std::default_random_engine generator(time(NULL));
std::uniform_int_distribution<int> distribution;
std::uniform_int_distribution<int> distribution_bin;
void getGradient(Mat& src, Mat& mag, Mat& ang)
{
    Mat src_g;
    cvtColor(src, src_g, COLOR_BGR2GRAY);
    src_g.convertTo(src_g, CV_32FC1,1.0/255.0);
    //GaussianBlur(src_g, src_g, Size(3, 3), 2);
    Mat gx, gy;
    Sobel(src_g, gx, -1, 1, 0);
    Sobel(src_g, gy, -1, 0, 1); //Gy smoothing + differention
    magnitude(gx, gy, mag); //
    phase(gx, gy, ang, true); //calculates rotation angle of 2D vectors
}
//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
class snow
{
public:
    float threshold;
    float x;
    float y;
    float t;
    float phase;
    bool dead;//if true snow flake stops moving
    snow()
    {
        threshold = 0.5;
        x = distribution(generator);//generate random first location for snow 
        phase = distribution(generator);
        y = 0;//upper side of the video
        dead = false;
    }

    void step(Mat& canvas, Mat& res_dead, Mat& mag, Mat& ang)
    {
        if(x<1)
        {
            x = 1;
        }
        if (x>canvas.cols-2)
        {
            x = canvas.cols-2;
        }

        float g = mag.at<float>(y+1, x);//returns the specified array element
        float a = ang.at<float>(y+1, x);//same as above

        float rdL = res_dead.at<Vec3b>(y + 1, x-1)[0];//same as above
        float rdM = res_dead.at<Vec3b>(y + 1, x)[0];//same as above
        float rdR = res_dead.at<Vec3b>(y + 1, x+1)[0];//same as above

        if (rdL > 0  && rdM > 0 && rdR == 0)
        {
            g = 10;
            a = 20;
        }
        if (rdL == 0 && rdM > 0 && rdR > 0)
        {
            g = 10;
            a = 140;
        }
        if (rdL > 0 && rdM > 0 && rdR > 0)
        {
            dead = true;
            return;
        }
        if (rdL == 0 && rdM > 0 && rdR == 0)
        {
            int des= distribution_bin(generator);
            if (des == 0)
            {
                a = 20;
            }
            else
            {
                a = 140;
            }
        }

        if (g < threshold && rdM==0)
        {
            ++y;
            x += 1 * sin(t/10+phase);
        }
        ++t;
        if (g > threshold)
        {
            if (a < 45 && a > 10)
            {
                ++y;
                ++x;
            }else if (a > 135 && a < 170)
            {
                ++y;
                --x;
            }
            else
            {
                ++y;
                dead = true;
                return;
            }
        }

        if (x > mag.cols - 1)
        {
           // ++y;
            x = mag.cols - 2;
            dead = true;
            return;
        }
        if (x < 0)
        {
            x = 1;
            dead = true;
            return;
        }
        if (y < 0)
        {
            y = 0;
            dead = true;
            return;
        }
        if (y >= mag.rows - 1)
        {
            y = mag.rows - 1;
            dead = true;
            return;
        }
    }
};
//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
Mat src;
Mat res;
Mat res_dead;
Mat mag, ang;
Mat mag_s;
Mat ang_s;
Mat mag_sum, ang_sum;
vector<snow> s(100);
int key = 0;
VideoWriter video;
//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------

void snowing(Mat src) {
	getGradient(src, mag, ang);
	Mat res = Mat::zeros(src.size(), src.type());
	res_dead = Mat::zeros(src.size(), src.type());
	mag_s= Mat::zeros(src.size(), CV_32FC1);
	ang_s = Mat::zeros(src.size(), CV_32FC1);
	distribution = std::uniform_int_distribution<int>(0,src.cols-1);
	distribution_bin = std::uniform_int_distribution<int>(0, 1);
	mag_sum = mag + mag_s;
	ang_sum = ang + ang_s;
	for (int k = 0; k < 20; ++k)
	{
	    snow sn;
	    s.push_back(sn);
	}
	for (int j = 0; j < s.size(); ++j)
	{
	    s[j].step(res, res_dead, mag_sum, ang_sum);
	    if (s[j].y < 0)
	    {
	        s[j].y = 0;
	    }
	    res.at<Vec3b>(s[j].y, s[j].x) = Vec3b(255, 255, 255);
	    if (s[j].dead)
	    {
	        res_dead.at<Vec3b>(s[j].y, s[j].x) = Vec3b(255, 255, 255);
	        s.erase(s.begin() + j);
	    }
	}
	res += res_dead;
	imshow("res", res+src);
	video.write(res+src);
	char c = (char)waitKey(33);
       	if( c == 27 ) exit(0);
	key=waitKey(10);
	res = Mat::zeros(src.size(), src.type());
}

//-----------------------------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------------------------
int main(int argc, unsigned int** argv)
{

	
	VideoCapture cap(0); // open the default camera
        if(!cap.isOpened())  // check if we succeeded
       		return -1;
	 video.open("out.mkv",  CV_FOURCC('M', 'J', 'P', 'G'), 25, src.size(), true);
        
	    
        while(key!=27)
	{
		Mat frame;
		cap >> frame;
		snowing(frame);		
        }
	 
	
	key=waitKey(0);
}
