#ifndef OBJECT_RECOGNITION_HPP
#define OBJECT_RECOGNITION_HPP

#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/opencv.hpp"
#include <vector>

class Image {
    cv::Mat m_objects;
    std::string m_out_path;
    uchar m_zlimit;

  public:
    cv::Mat image;
    std::vector<cv::Mat> mask_mats;

  public:
    Image(std::string path, std::string output_location);

    void write(std::string path);

    cv::Mat erode(int erosion_dst, int erosion_size);

    cv::Mat dilate(int dilation_dst, int dilation_size);

    void findObjects(uchar zlimit = 10, int minDots = 1000, int maxObjects = 5);

    void findObjectsIterative(uchar zlimit = 10, int minDots = 1000,
                              int maxObjects = 5);

  private:
    int directions[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};

    struct Dirs {
        cv::Point RIGHT{1, 0};
        cv::Point DOWN{0, 1};
        cv::Point LEFT{-1, 0};
        cv::Point UP{0, -1};

        std::vector<cv::Point> toRIGHT{Dirs::RIGHT, Dirs::DOWN, Dirs::UP};
        std::vector<cv::Point> toDOWN{Dirs::RIGHT, Dirs::DOWN, Dirs::LEFT};
        std::vector<cv::Point> toLEFT{Dirs::DOWN, Dirs::LEFT, Dirs::UP};
        std::vector<cv::Point> toUP{Dirs::RIGHT, Dirs::LEFT, Dirs::UP};

        std::vector<cv::Point> nextDirectionList(cv::Point current_direction) {
            if (current_direction == Dirs::RIGHT) {
                return toRIGHT;
            } else if (current_direction == Dirs::DOWN) {
                return toDOWN;
            } else if (current_direction == Dirs::LEFT) {
                return toLEFT;
            } else if (current_direction == Dirs::UP) {
                return toUP;
            }
        }
    };

    bool walk(cv::Mat &output, uchar prev_z, int x, int y, uchar &id,
              int &visited, int &amount);

    cv::Mat paint(cv::Point start, uchar &id, int &visited, int &amount);

    void iterate(cv::Point start, cv::Mat &output, int imageLeft, uchar &id,
                 int &visited, int &amount);

    void printTimeTaken(std::chrono::microseconds time_taken,
                        std::string function_name);

    void log_system_stats(std::chrono::microseconds time_taken,
                          std::string function_name, int version, int batch,
                          std::string log_name);
};

#endif