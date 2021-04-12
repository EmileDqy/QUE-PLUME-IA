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

#if !__APPLE__
  #include <wiringPi.h>
  #define RED_LED 0
  #define GREEN_LED 2
#endif

using namespace std; // Not really recommanded... but it's a prototype. Not for prod.
using namespace cv;  // OpenCV 4.5.0 (latest)

// Tracker's position
int old_x;
int old_y;

int64 old_timer;
double wait_time_trigger = 1; // second
double speed_tracker;
float threshold_speed = 3.0;

int object_w = 80;
int object_h = 80;
int safe_zone_origin_width = 100;
int safe_zone_origin_height = 100;

bool triggered = false;

int timer_AI = 5;
dnn::Net net = dnn::readNetFromTensorflow(
  "./frozen_models/frozen_graph_v12.pb", 
  "./frozen_models/frozen_graph_v12.pbtxt"
);

Rect bounding_box_object;

const char *classes[4] = { "goupille", "couvercle", "boite", "Autre" };

/**
 * Function whose goal is to execute as a subprocess
 * a command.
 */
void exec(const char* cmd) {
    array<char, 128> buffer;
    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw runtime_error("popen() failed!");
    }
}

/**
 * Main function for this file.
 * The goal is to :
 *  - Track the robot's hand
 *  - Extract the object from the frame
 *  - Make some estimations about its shape and color and send the data to our database
 */
void trackAndRecognize(){
  
  #if !__APPLE__
    wiringPiSetup();
    pinMode(RED_LED, OUTPUT) ; // GPIO 17 -> Rouge
    pinMode(GREEN_LED, OUTPUT) ; // GPIO 27 -> Vert
  #endif

  vector<string> regionsNames;
  for(int i = 0; i < elements.size(); i++){
    vector<string> s = elements[i];
    string s1 = s[0] + "_RGB(" + s[1] + "," + s[2] + "," + s[3] + ")";
    regionsNames.push_back(s1);
  }

  // set input video
  VideoCapture cam(0);
  Mat frame;
  cam >> frame; // skip 
  cam >> frame; // skip
  cam >> frame; // take

  // create a tracker object
  Ptr<Tracker> tracker = TrackerCSRT::create();
  Rect roi;
  roi=selectROI("Camera",frame); // Ask for a ROI (something to track) : Robot's arm.

  // If no ROI, that's an error.
  if(roi.width==0 || roi.height==0)
    return;

  tracker->init(frame,roi);
  old_timer = getTickCount();
  
  // Load the different storage regions
  Mat images[elements.size()];
  for(int i = 0; i<6; i++) {
    images[i] = imread("./mask_calibration_" + regionsNames[i] + ".png", IMREAD_GRAYSCALE);
  }
  
  // Init the variable of timer of the AI
  int64 triggerTime;
  
  // For each frame
  for ( ;; ){
    cam >> frame;

    if(frame.rows==0 || frame.cols==0)
      continue;

    tracker->update(frame,roi);

    // Fetch coordinates of the ROI
    int64 t = getTickCount();
    if(t - old_timer >= getTickFrequency()*wait_time_trigger && !triggered){
      speed_tracker = sqrt(abs(roi.x - old_x) + abs(roi.y - old_y));
      old_x = roi.x;
      old_y = roi.y;
      old_timer = t;
      
      // Display the speed of the tracker
      std::ostringstream strs;
      strs << speed_tracker;
      std::string str = strs.str();
      cout << "Tracker's speed:\t" << str << "\tpx/s" << endl;

      // Trigger the timer of the AI + set rect to value if speed <= threshold_speed 
      if(speed_tracker <= threshold_speed){
        // Defines the bounding box to display
        bounding_box_object = Rect(
          roi.x - (object_w-roi.width )/2,
          roi.y - (object_h-roi.height)/2,
          object_w, 
          object_h
        );
        // Init the timer of the AI
        triggered = true;
        triggerTime = getTickCount(); 
      }
    }

    if(old_x <= safe_zone_origin_width && old_y <= safe_zone_origin_height){
      triggered = false;
    } else if(triggered && t - triggerTime >= getTickFrequency()*timer_AI) { // Wait N seconds if we need to trigger
      
      // We extract our object from the frame
      cv::Mat extract(
        frame, // Frame to copy
        Range( bounding_box_object.y, bounding_box_object.y + bounding_box_object.height), // Y
        Range( bounding_box_object.x, bounding_box_object.x + bounding_box_object.width)   // X
      );

      // We resize our frame to the right size.            
      resize(extract, extract, Size(224, 224));

      // We transform our frame into a blob (vector w/ 4 dim.)
      Mat blob;
      dnn::blobFromImage(extract, blob, (1.0/255), Size(224, 224), Scalar(0), true, true);      
      imwrite("input_AI.png", extract);

      // The blob will be the input for our AI
      net.setInput(blob);
      
      // Now, we forward our blob and get the prediction
      Mat output = net.forward();
      
      // Convert from Mat to vector<float>
      vector<float> v;
      output.row(0).copyTo(v);

      // Get the index of the highest prediction
      int maxElementIndex = std::max_element(v.begin(),v.end()) - v.begin();
      
      // Display results : type of object
      cout << "Vecteur estimé: " << output << endl;
      cout << "Estimation: " << classes[maxElementIndex] << endl;
      cout << flush;
      
      // What's left to do now is to get the color from the object
      vector<int> color = getColor(extract);
      cout << "Object Color RGB(" << color[0] << "," << color[1] << "," << color[2] << ");" << endl;
      
      // For each storage region, we check wheither or not our object is inside. 
      // We also check wheither or not the color and shape correspond.
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
          }
        }else{
          cout << "Error: Couldn't find any contours for region " \
                << elements[i][0]                     \
                << " - RGB(" << elements[i][1] << "," \
                << elements[i][2] << ","              \
                << elements[i][3] << ")"              \
                << endl;
        }
      }

      // If the object is in the right region, we say it, swith the LEDs 
      if(isCorrect) {
        cout << "Correct shape and color. Good!" << endl;
        cout << elements[index_region_inside][0] << endl;
        
        #if !__APPLE__
          digitalWrite(RED_LED, LOW);
          digitalWrite(GREEN_LED, HIGH);
        #endif

      } else {
        
        #if !__APPLE__
          digitalWrite(GREEN_LED, LOW);
          digitalWrite(RED_LED, HIGH);
        #endif

        if(index_region_inside != -1) {
          cout << "Something is wrong. Expected:"                       \
                << elements[index_region_inside][0]                     \
                << " - RGB(" << elements[index_region_inside][1] << "," \
                << elements[index_region_inside][2] << ","              \
                << elements[index_region_inside][3] << ")"              \
                << ". But got : "                                       \
                << classes[maxElementIndex] << "' "                     \
                << "'RGB(" << color[0] << "," << color[1] << "," << color[2] << ")'" \
                << endl;
        }
      }

      // Now, we need to send our data to the database
      if(index_region_inside != -1) { 
        std::ostringstream cmd;
        cmd << "/usr/bin/python3 ./datapush.py '";
        cmd << elements[index_region_inside][0]            << "' "  ;             // type_expected
        cmd << "'RGB(" << elements[index_region_inside][1] << ","   ;             // |
        cmd << elements[index_region_inside][2]            << ","   ;             // |
        cmd << elements[index_region_inside][3]            << ")' '";             // color_expected
        cmd << classes[maxElementIndex]                    << "' "  ;             // type_found
        cmd << "'RGB(" << color[0] << "," << color[1] << "," << color[2] << ")'"; // color_found
        
        cout << cmd.str() << endl; // Display the command
        exec(cmd.str().c_str());   // We start the script python for firebase
      }

      triggered = false; // Turn off the timer of the AI
    }

    rectangle(frame, roi, Scalar( 255, 10, 90 ), 2, 2 );         // Display the tracker
    rectangle(frame, bounding_box_object, Scalar(255, 0, 0), 1); // Display the bounding box

    imshow("Camera",frame); // Display the frame

    if((waitKey(1) & 0xFF) == 'q') break;
  }
}
