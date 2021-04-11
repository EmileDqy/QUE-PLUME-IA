#include <iostream>

#include "core.h"
#include <cstring>
#include <cstdlib>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/utility.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <vector>

using namespace cv;
using namespace std;

// Noir, blanc, rouge, gris, fond blanc
vector<int> blanc   = {255, 255, 255};
vector<int> noir    = {0  ,   0,   0};
vector<int> rouge   = {255,   0,   0};
vector<int> gris    = {127, 127, 127};
vector<int> fond    = {255, 255, 255};
vector<vector<int>> colors = {noir, blanc, rouge, fond };

std::vector<int> getColor(Mat frame1)
{
    cout << "Extracting predominant color from object..." << endl;
    
    Mat frame;
    frame1.copyTo(frame);
    cvtColor(frame, frame, COLOR_BGR2RGB);
    Mat samples(frame.rows * frame.cols, 3, CV_32F);
    for( int y = 0; y < frame.rows; y++ )
        for( int x = 0; x < frame.cols; x++ )
            for( int z = 0; z < 3; z++)
                samples.at<float>(y + x*frame.rows, z) = frame.at<Vec3b>(y,x)[z];

    int clusterCount = 2; //On veut 2 couleurs
    Mat labels;
    int attempts = 1;
    Mat centers;
    // kmeans pour trouver le centre de nos clusters
    kmeans(samples, clusterCount, labels, TermCriteria(TermCriteria::MAX_ITER+TermCriteria::EPS, 1, 0.11), attempts, KMEANS_PP_CENTERS, centers );

    cout << "Two centroids were detected:" << endl;
    cout << centers << endl;
    cout << "Calculating the nearest colors in our palette from the euclidian distances in RGB space..." << endl;

    //Calcul de la distance pour chaque cluster center entre sa couleur et celles de la matrice colors
    vector<vector<int>> colors_detected;
    for(int i = 0; i < clusterCount; i++){
        float min = -1;
        vector<int> label;
        for(int j = 0; j < colors.size(); j++){
            
            float dist = sqrt(\
                powf(centers.at<float>(i, 0) - colors[j][0], 2) +\
                powf(centers.at<float>(i, 1) - colors[j][1], 2) +\
                powf(centers.at<float>(i, 2) - colors[j][2], 2)  \
            );

            if(dist < min || min == -1){
                min = dist;
                label.clear();
                label.push_back(colors[j][0]);
                label.push_back(colors[j][1]);
                label.push_back(colors[j][2]);   
            }

        }
        colors_detected.push_back(label);
    }

    vector<int> c0 = colors_detected[0];
    vector<int> c1 = colors_detected[1];

    // We check weither or not our first cluster is the background color. If the two are inversed, we swap.
    if(c0 == fond || c1 == fond){
        cout << "Background RGB("<< fond[0] << "," << fond[1] << "," << fond[2] << ") detected." << endl;
    }
    if(c0 != fond && c1 == fond){
        cout << "Wrong order. Swapping clusters." << endl;
        vector<int> temp = c0;
        c0 = c1;
        c1 = temp;
    }else if(c0 != fond && c1 != fond){ // If neither of them corresponds to the background color, we end here.
        cout << "Error: bad colors for image. No background detected. Skipping." << endl;
    }

    // Now, we only have to return the color of c1
    cout << "All good. Final color detected: RGB(" << c1[0] << "," << c1[1] << "," << c1[2] << ")" << endl;
    return c1;
}
