#include <stdio.h>
#include <opencv2/opencv.hpp>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include <string> 

//cmake . && make && ./DisplayImage
using namespace cv;
using namespace std;

VideoCapture cam(1);
int thresh =80 ;
RNG rng(12345);

void waitForCam(){
    while (!cam.isOpened()) {
        cout << "Failed to make connection to cam" << endl;
        cam.open(1);
    }
}

void contrastStretch(Mat &srcImage){
    double pixMin,pixMax;
    minMaxLoc(srcImage,&pixMin,&pixMax);

    Mat lut( 1, 256, CV_8U);
    for( int i = 0; i < 256; i++ ){
        if (i < pixMin) lut.at<uchar>(i)= 0;
        else if (i > pixMax) lut.at<uchar>(i)= 255;
        else lut.at<uchar>(i)= static_cast<uchar> (255.0*(i-pixMin)/(pixMax-pixMin)+0.5);
    }
    LUT( srcImage, lut, srcImage );
}

int main(){
    Mat pic;
    Mat pic_blured;
    Mat pic_depth;
    Mat pic_gray;
    Mat pic_gray_blured;
    Mat pic_sharpened;
    Mat canny_output;
    Mat pic_dilated;
    Mat pic_morphed;

    Scalar color = Scalar(255, 0, 50);
    
    waitForCam();
    while(true){
        int number_squares = 0;
        cam>>pic;
        resize(pic, pic, Size(pic.size().width * 0.5, pic.size().height * 0.5));
        
        contrastStretch(pic);
        
        GaussianBlur(pic, pic_blured, Size(5, 5), 0, 0, BORDER_DEFAULT);
        
        imshow("Picture", pic_blured);
        
        cvtColor(pic_blured, pic_gray, COLOR_RGB2GRAY);
        
        Mat final = Mat::zeros( pic_gray.size(), CV_8UC3 );       
 

        Canny(pic_gray, canny_output, thresh, 180, 3, true);
        
        cv::Mat kernel = cv::Mat(2, 2, CV_8U);
        morphologyEx(canny_output, pic_morphed, MORPH_CLOSE, kernel);
        //pic_morphed = canny_output.clone();

        cv::Mat kernel_dilate = cv::Mat(3, 3, CV_8U);
        dilate(pic_morphed, pic_morphed, kernel_dilate);
        
        vector<vector<Point> > contours;
        vector<Vec4i> hierarchy;
        findContours( pic_morphed, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

        for( size_t i = 0; i< contours.size(); i++ )
        {
            double len = arcLength(contours[i], true);
            vector<Point> cnt;
            approxPolyDP(contours[i], cnt, 0.009 *len, true); 
            if((cnt.size() == 4 || cnt.size() == 4) && contourArea(cnt) > 200 && isContourConvex(cnt) ){
                vector<vector<Point> > cnt_container;
                cnt_container.push_back(cnt);
                number_squares++;
                fillPoly(final, cnt_container, color); // Here because sometimes we detect multiple times the same square and fillpoly cannot draw overlapping polygons
            }
        }
        setWindowTitle("camera", "Squares: " + to_string(number_squares));
        imshow("camera", final);
        if(waitKey(30) != -1){
            break;
        }
    }
}
