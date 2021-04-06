#include <iostream>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <array>

#include "core.h"

#include <opencv2/core/utility.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/dnn.hpp>

#include <wiringPi.h>

#define RED_LED 0
#define GREEN_LED 2

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

const char *classes[4] = { "boite", "couvercle", "goupille", "Autre" };

int num_storage = 6; // 6 storage

void exec(const char* cmd) {
    array<char, 128> buffer;
    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw runtime_error("popen() failed!");
    }
}


void trackAndRecognize(){
  
  wiringPiSetup();
  pinMode(RED_LED, OUTPUT) ; // GPIO 17 -> Rouge
  pinMode(GREEN_LED, OUTPUT) ; // GPIO 27 -> Vert

  vector<string> regionsNames;
  for(int i = 0; i < elements.size(); i++){
    vector<string> s = elements[i];
    string s1 = s[0] + "_RGB(" + s[1] + "," + s[2] + "," + s[3] + ")";
    regionsNames.push_back(s1);
  }

  Rect roi;
  Mat frame;

  // create a tracker object
  Ptr<Tracker> tracker = TrackerCSRT::create();

  // set input video
  VideoCapture cam(0);
  cam.set(CAP_PROP_AUTOFOCUS, 0);
    
  // We need to wait a little for the cam to start.
  cam >> frame; // skip 
  cam >> frame; // skip
  cam >> frame; // take

  // Ask for a ROI (something to track)
  roi=selectROI("Camera",frame);

  if(roi.width==0 || roi.height==0)
    return;

  tracker->init(frame,roi);
  old_timer = getTickCount();
  
  Mat images[6];
  for(int i = 0; i<6; i++)
  {
    images[i] = imread("./mask_calibration_" + regionsNames[i] + ".png", IMREAD_GRAYSCALE);
  }

  cout << elements.size() << endl;
  
  int64 triggerTime;
  for ( ;; ){
    cam >> frame;

    if(frame.rows==0 || frame.cols==0)
      continue;

    tracker->update(frame,roi);

    // Fetch coordinates of the ROI
    int64 t = getTickCount();
    if(t - old_timer >= getTickFrequency()*wait_time_trigger && !triggered){
      diff = sqrt(abs(roi.x - old_x) + abs(roi.y - old_y));
      old_x = roi.x;
      old_y = roi.y;
      old_timer = t;
      
      std::ostringstream strs;
      strs << diff;
      std::string str = strs.str();
      cout << "Tracker's speed:\t" << str << "\tpx/s" << endl;

      if(diff <= 3.0){
        triggered = true;
        rect = Rect(roi.x-(object_w-roi.width)*0.5, roi.y-(object_h-roi.height)*0.5, object_w, object_h);
        triggerTime = getTickCount();
      }

    }
    
    rectangle(frame, roi, Scalar( 255, 10, 90 ), 2, 2 );

    // No trigger zone
    //rectangle(frame, Rect(Point(1, 1), Point(250, 250)), Scalar( 255, 255, 255 ), 1, 2);

    if(old_x <= 100 && old_y <= 100){
      triggered = false;
    } else if(triggered && t - triggerTime >= getTickFrequency()*5) { // Wait 5 seconds if we need to trigger
      Range r1 = Range( old_y, old_y+object_h );
      if(r1.start <= 0) r1.start = 1;
      if(r1.end > frame.rows) r1.end = frame.rows;
      Range r2 = Range( old_x-object_w, old_x );
      if(r2.start <= 0) r2.start = 1;
      if(r2.end > frame.cols) r2.end = frame.cols;
      
      cout << r1 << " " << r2 << endl;

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
      
      vector<int> color = getColor(extract);
      cout << "Object Color RGB(" << color[0] << "," << color[1] << "," << color[2] << ");" << endl;
      
      // Crash after this line
      bool isCorrect = false;
      int index_region_inside = -1;

      for(int i = 0; i < elements.size(); i++){
        int rx = roi.x;
        int ry = roi.y;
        int expected_R = std::stoi(elements[i][1]);
        int expected_G = std::stoi(elements[i][2]);
        int expected_B = std::stoi(elements[i][3]);
        
        vector<vector<Point> > contours;
        vector<Vec4i> hierarchy;
        findContours(images[i], contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
        
        vector<Point> cnt;
        if(contours.size() > 0){

          double len = arcLength(contours[0], true);
          approxPolyDP(contours[0], cnt, 0.009 *len, true);
        
          vector<vector<Point>> shapes = {cnt};
          drawContours(frame, shapes, 0, Scalar(0, 0, 255), 2);
        
          int res = pointPolygonTest(cnt, Point(rx, ry), false);

          if(res == 1 || res == 0){ //If the point is inside 
            cout << "res=" << res << " i=" << i << endl;
            index_region_inside = i;
            if(classes[maxElementIndex] != elements[i][0]){
              continue;
            }

            if(color[0] != expected_R || color[1] != expected_G || color[2] != expected_B){
              continue;
            }

            isCorrect = true;
            break;
          
          }else{
            cout << "Object not in region "            \
                << elements[i][0]                     \
                << " - RGB(" << elements[i][1] << "," \
                << elements[i][2] << ","              \
                << elements[i][3] << ")"              \
            << endl;
          }

        }else{
          cout << "Error: Couldn't find any shape for given region: " \
                << elements[i][0]                     \
                << " - RGB(" << elements[i][1] << "," \
                << elements[i][2] << ","              \
                << elements[i][3] << ")"              \
                << endl;
        }
      }

      if(isCorrect) {
        cout << "Correct shape and color. Good!" << endl;
        digitalWrite(RED_LED, LOW);
        digitalWrite(GREEN_LED, HIGH);
      } else {
        digitalWrite(GREEN_LED, LOW);
        digitalWrite(RED_LED, HIGH);
        if(index_region_inside != -1) {
          cout << "Something is wrong. Expected:"                      \
              << elements[index_region_inside][0]                     \
              << " - RGB(" << elements[index_region_inside][1] << "," \
              << elements[index_region_inside][2] << ","              \
              << elements[index_region_inside][3] << ")"              \
          << endl;
        }
      }
      triggered = false;

      
        std::ostringstream cmd;
        cmd << "/usr/bin/python3 ./datapush.py '";
        
        cmd << elements[index_region_inside][0] << "' "; // type_expected
        cmd << "'RGB(" << elements[index_region_inside][1] << "," \
            << elements[index_region_inside][2] << ","              \
            << elements[index_region_inside][3] << ")' '"; // color_expected
        
        cmd << classes[maxElementIndex] << "' "; // type_found
        cmd << "'RGB(" << color[0] << "," << color[1] << "," << color[2] << ")'"; // color_found
        
        cout << cmd.str() << " <<" << endl; 
        exec(cmd.str().c_str());
    }

    rectangle(frame, rect, Scalar(255, 0, 0), 1);
    
    imshow("Camera",frame);
    if((waitKey(1) & 0xFF) == 'q') break;
  }
}
