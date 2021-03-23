#ifndef HEADER_FILE_H
#define HEADER_FILE_H

#include <vector>
#include <opencv2/core/utility.hpp>

void calibrate();
void trackAndRecognize();
std::vector<int> getColor(cv::Mat frame1);
namespace { std::vector<std::vector<cv::Point>> vector_regions; }
extern std::vector<std::vector<cv::Point>> vector_regions;

#endif // HEADER_FILE_H 

