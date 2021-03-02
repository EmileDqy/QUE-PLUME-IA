#include <opencv2/core/utility.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <cstring>

using namespace std;
using namespace cv;

int main( int argc, char** argv ){
  
  Rect roi;
  Mat frame;

  // create a tracker object
  Ptr<Tracker> tracker = TrackerCSRT::create();
  
  // set input video
  VideoCapture cam(1);
  
  cam >> frame;
  cam >> frame;

  // Ask for a ROI (something to track)
  roi=selectROI("tracker",frame);

  if(roi.width==0 || roi.height==0)
    return 0;

  tracker->init(frame,roi);

  for ( ;; ){
    cam >> frame;

    if(frame.rows==0 || frame.cols==0)
      continue;

    tracker->update(frame,roi);

    rectangle( frame, roi, Scalar( 255, 150, 90 ), 2, 2 );

    imshow("tracker",frame);

    if(waitKey(1)==27)break; // ESC
  }
  return 0;
}