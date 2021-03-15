#include <iostream>
#include <cstring>
#include <cstdlib>

#include "core.h"

int main(int argc, char const *argv[])
{
    std::cout << "Initializing calibration." << std::endl;
    calibrate();
    std::cout << "Running Tracker." << std::endl;
    trackAndRecognize();
    return 0;
}
