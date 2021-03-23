#include <iostream>

#include "core.h"

#include <opencv2/core/utility.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/dnn.hpp>
#include <iostream>
#include <cstring>
#include <cstdlib>


// Declaring our namespaces... std shouldn't be here, it's not recommended but I'm lazy
using namespace cv;
using namespace std;

// Initialization of our matrices containing our different images.s
Mat frame;
Mat frame_blur;
Mat frame_gray;
Mat canny;
Mat frame_morphed;
Mat pic;

// Our canny threshold, the one we modify with our trackbar.
int lowThreshold = 30; //% ; 30 is a good value

// We initialize two vectors of contours. 
//   - One for the currently and potentialy valid regions.
//   - The other for the region that we selected.
vector<vector<Point>> regions_displayed;
vector<vector<Point>> regions_saved;

// The different names of our storages.
string storage_names[] = {
    "boite_noire",
    "boite_blanche",
    "couvercle_noir",
    "couvercle_blanc",
    "goupille_noire",
    "goupille_blanche"
};

// We use this fonction to strech the sprectrum of our image.
// It helps for edge detection because is enhances the contrast.
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

// Called when the mouse iteracts witht the window (highgui)
void onMouse(int event, int x, int y, int flags, void* userdata){
    // If our event is a left click and we still have regions to select
    if(event == EVENT_LBUTTONDOWN && regions_saved.size() < sizeof(storage_names)/sizeof(storage_names[0])){
        
        vector<vector<Point>> contour; // We initialize our contour vector

        // First, we check wheither or not our click is inside a region displayed in blue (detected this frame).
        int isInsideRegion = 0; // self described, but cannot be a bool because pointPolygonTest returns either -1, 0 or 1.
        for(int i = 0; i < regions_displayed.size(); i++){
            isInsideRegion = pointPolygonTest(regions_displayed[i], Point2f(x, y), false); 
            if(isInsideRegion == 1){
                contour.push_back(regions_displayed[i]);
                break;
            }
        }

        // Then, we check wheither or not the click was inside an already saved quadrilateral. We don't want to save the same one two times.
        int isInsideRegionSaved = 0;
        if(isInsideRegion == 1){
            for(int i = 0; i < regions_saved.size(); i++){
                isInsideRegionSaved = pointPolygonTest(regions_saved[i], Point2f(x, y), false);
                if(isInsideRegionSaved == 1){
                    break;
                }
            }
        }

        // If it's ok, we add the contour to our vector containing our saved regions (in red).
        if(isInsideRegion == 1 && isInsideRegionSaved != 1) regions_saved.push_back(contour[0]);
    }
}

void calibrate()
{
    string window_title = "Camera";

    VideoCapture cam(0); // We initialize our cam and ask for the peripheral number 1 (only MacOS, 0 otherwise)

    for(;;){
        cam >> frame; // We fetch our image from the camera.
        
        if(regions_saved.size() < sizeof(storage_names)/sizeof(storage_names[0]))
            window_title = storage_names[regions_saved.size()]; // We display the name of the next region to be selected.
        else
            break; // When we have selected the number of regions needed, we break the loop. We then display the result with the real image (frame).
        setWindowTitle("Camera", window_title);        

        // Simple image processing
        contrastStretch(frame);
        GaussianBlur(frame, frame_blur, Size(5, 5), 0, 0, BORDER_DEFAULT);
        cvtColor(frame_blur, frame_gray, COLOR_RGB2GRAY);
        Canny(frame_gray, canny, lowThreshold*255/100, lowThreshold*2*255/100, 3);
        cvtColor(canny, pic, COLOR_GRAY2RGB); // Pic is our final image, the one displayed
        
        // We create the TrackBar, it's needed if we want to calibrate our canny filter and thus have a good detection of our storage regions.
        createTrackbar( "Min Threshold:", "Camera", &lowThreshold, 100);
        stringstream str;
        string s;
        str << lowThreshold*255/100;
        str >> s;
        putText(pic, "Threshold: " + s + "%", Point(20, 20), FONT_HERSHEY_PLAIN, 1, Scalar(0, 0, 255));

        regions_displayed.clear(); // Because we update our regions detected each frame, we need to clear our vector.

        vector<vector<Point> > contours;
        vector<Vec4i> hierarchy;
        findContours( canny, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE); // We find the contours on from our canny filter.

        for( size_t i = 0; i< contours.size(); i++ )
        {
            double len = arcLength(contours[i], true);
            vector<Point> cnt;
            approxPolyDP(contours[i], cnt, 0.009 *len, true); // polygon approximation of the contour of index i
            if(cnt.size() == 4 && contourArea(cnt) > 200 && isContourConvex(cnt) ){ // quadrilateral detection
                regions_displayed.push_back(cnt); // we add it to the regions to be displayed (in blue)
            }
        }
        
        setMouseCallback("Camera", onMouse);
        
        //Draw on our picture
        drawContours(pic, regions_displayed, -1, Scalar(255, 0, 0), 3);
        drawContours(pic, regions_saved,     -1, Scalar(0, 0, 255), 5);
        for(int i = 0; i < regions_saved.size(); i++)
            putText(pic, storage_names[i], Point(regions_saved[i][0].x-15, regions_saved[i][0].y-5), FONT_HERSHEY_PLAIN, 1, Scalar(0, 0, 255));
        
        imshow("Camera", pic);
        if((waitKey(1) & 0xFF) == 'q') break; // From here, we can simply read our masks and get our regions and their names. 
        // Because breaking here leads to the camera.
        //                  |
        //                  V
    }

    Mat mask = Mat::ones(pic.size(), CV_8U);
    drawContours(mask, regions_saved, -1, Scalar(0, 0, 255), 5);
    imwrite("./mask_calibration.png", mask*255); // After that, we fetch the image on the other side and apply approxPolyDP + 4 sides detection to get our
    // But we'll have trouble getting the names right there, so we might as well each region individualy with their corresponding name.

    // Here we just write the names because we need to display the camera with the mask on it.
    for(int i = 0; i < regions_saved.size(); i++) {
        Mat mask_local = Mat::ones(pic.size(), CV_8U);
        putText(mask, storage_names[i], Point(regions_saved[i][0].x-15, regions_saved[i][0].y-5), FONT_HERSHEY_PLAIN, 1, Scalar(0, 0, 255));
        drawContours(mask_local, regions_saved, i, Scalar(0, 0, 255), 5);
        imwrite("~/QUE-PLUME-IA/mask_calibration_" + storage_names[i] + ".png", mask_local * 255);
    }
    
    std::vector<std::vector<cv::Point>> vector_regions = regions_saved;

    // We simply apply the mask on the frame (direct output of our camera).
    Mat final;
    for(;;){
        cam >> frame;
        bitwise_and(frame, frame, final ,mask = mask);
        imshow("Camera", final); // We see our regions and the world is normal, no longer in canny.
        if((waitKey(1) & 0xFF) == 'q') break;
    }
}
