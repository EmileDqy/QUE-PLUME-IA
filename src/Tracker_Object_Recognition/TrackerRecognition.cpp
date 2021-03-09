#include <opencv2/core/utility.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/dnn.hpp>
#include <iostream>
#include <cstring>
#include <cstdlib>

#include "manager.h"

using namespace std;
using namespace cv;

int old_x;
int old_y;
int64 old_timer;
double diff;

int object_w = 150;
int object_h = object_w;

bool triggered = false;
bool one_take = false;
double wait_time_trigger = 1; // second
Rect rect;

const char *classes[4] = { "Boite", "Couvercle", "Goupille", "Autre" };

int num_storage = 6; // 6 storage

int trackAndRecognize( int argc, char** argv ){
  
  Rect roi;
  Mat frame;

  // create a tracker object
  Ptr<Tracker> tracker = TrackerCSRT::create();
  
  // set input video
  VideoCapture cam(0);
  
  // We need to wait a little for the cam to start.
  cam >> frame; // skip 
  cam >> frame; // skip
  cam >> frame; // take

  // Ask for a ROI (something to track)
  roi=selectROI("tracker",frame);

  if(roi.width==0 || roi.height==0)
    return 0;

  tracker->init(frame,roi);
  old_timer = getTickCount();

  for ( ;; ){
    cam >> frame;

    if(frame.rows==0 || frame.cols==0)
      continue;

    tracker->update(frame,roi);

    int64 t = getTickCount();
    if(t - old_timer >= getTickFrequency()*wait_time_trigger && !triggered){
      diff = sqrt(abs(roi.x - old_x) + abs(roi.y - old_y));
      old_x = roi.x;
      old_y = roi.y;
      old_timer = t;
      
      std::ostringstream strs;
      strs << diff;
      std::string str = strs.str();
      cout << str + "\n";

      if(diff <= 3.0){
        triggered = true;
        rect = Rect(roi.x-object_w, roi.y, object_w, object_h);
      }

    }
    
    rectangle(frame, roi, Scalar( 255, 10, 90 ), 2, 2 );
    
    if(triggered && !one_take){
      one_take = true;
      cv::Mat extract(
        frame,                              // Frame to copy
        cv::Range( old_y, old_y+object_h ), // Range along Y axis
        cv::Range( old_x-object_w, old_x )  // Range along X axis
      );
      
      resize(extract, extract, Size(224, 224));
      dnn::Net net = dnn::readNetFromTensorflow("./frozen_graph.pb");//, "./frozen_graph.pbtxt");
      net.setInput(dnn::blobFromImage(extract, (1.0), Size(224, 224)));
      Mat output = net.forward();
      
      vector<float> v;
      output.row(0).copyTo(v);
      int maxElementIndex = std::max_element(v.begin(),v.end()) - v.begin();
      
      cout << "Vecteur estimé: " << output << endl;
      cout << "Estimation: " << classes[maxElementIndex] << endl;
      cout << flush;
    }
    
    rectangle(frame, rect, Scalar(255, 0, 0), 1);
    
    imshow("tracker",frame);

    if(waitKey(1)==27)break; // ESC
  }
  return 0;
}