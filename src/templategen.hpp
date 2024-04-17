#ifndef TEMPLATEGEN_HPP
#define TEMPLATEGEN_HPP

#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/opencv.hpp"

class Templates {

    int chessboardSize = 20; // Size of each square in the chessboard
    int filler = 2 * chessboardSize;
    // int speedX = 1;
    // int speedY = 1;

    int width = 1280;
    int height = 720;

  public:
    Templates(cv::Mat mask) {}

    cv::Mat gradient(int iter, cv::Mat mask, int a) {}

    cv::Mat chessBoard(int iter, cv::Mat mask, int speedX = 1, int speedY = 1) {
    }
};

#endif