#ifndef OBJECT_RECOGNITION_HPP
#define OBJECT_RECOGNITION_HPP

#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/opencv.hpp"
#include <vector>

class Image {
    cv::Mat objects;
    std::string out_path;

  public:
    cv::Mat image;
    std::vector<cv::Mat> mask_mats;

  public:
    Image(std::string path, std::string output_location);

    void write(std::string path);

    cv::Mat erode(int erosion_dst, int erosion_size);

    cv::Mat dilate(int dilation_dst, int dilation_size);

    void findObjects(uchar z_limit = 10);

  private:
    int directions[4][2] = {{1, 0}, {0, 1}, {0, -1}, {-1, 0}};

    bool walk(cv::Mat &image, cv::Mat &objects, cv::Mat &output, uchar z_limit,
              uchar prev_z, cv::Point current, std::vector<cv::Point> path,
              uchar id);

    cv::Mat paint(cv::Mat &image, cv::Mat &objects, int z_limit,
                  cv::Point start, uchar id);
};

#endif