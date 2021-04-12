#ifndef HEADER_FILE_H
#define HEADER_FILE_H

#include <vector>
#include <opencv2/core/utility.hpp>

void calibrate();
void trackAndRecognize();
std::vector<int> getColor(cv::Mat frame1);
namespace { std::vector<std::vector<cv::Point>> vector_regions; }
extern std::vector<std::vector<cv::Point>> vector_regions;

static std::vector<std::vector<std::string>> elements = {
    {"boite", "0", "0", "0"},
    {"boite", "255", "255", "255"},
    {"couvercle", "0", "0", "0"},
    {"couvercle", "255", "255", "255"},
    {"goupille", "255", "0", "0"},
    {"goupille", "255", "255", "255"}
};

#endif // HEADER_FILE_H 

