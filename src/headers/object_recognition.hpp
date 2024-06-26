#ifndef OBJECT_RECOGNITION_HPP
#define OBJECT_RECOGNITION_HPP

#include <vector>

#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/opencv.hpp"
#include "utils.hpp"

struct Parameters {
    uchar z_limit = 10;
    uchar min_distance = 0;
    uchar medium_limit = 10;
    int min_area = 1000;
    int max_objects = 5;
    bool recurse = false;
};
// TODO restructure so that there would be initparams analog
class ImageProcessor {
    std::string m_out_path;
    Logger &m_log;
    Printer &m_printer;
    Parameters m_parameters;

   public:
    // TODO temp
    cv::Mat m_objects;

    cv::Mat *image;

    struct MatWithInfo {
        cv::Mat mat;
        int area = 0;
    };

    std::vector<MatWithInfo> mask_mats;

    struct Stats {
        int &isited;
        int &amount;
    };

   public:
    ImageProcessor(std::string output_location, Logger &log, Printer &printer);

    // TODO check if image on the path exists, throw exception if it doesn't
    ImageProcessor(std::string path, std::string output_location, Logger &log,
                   Printer &printer);

    ImageProcessor(cv::Mat image, std::string output_location, Logger &log,
                   Printer &printer);

    // TODO check if image is present before doing anything
    void getImage(std::string path);
    void getImage(cv::Mat *new_image);

    void write(std::string path);

    void setParametersFromSettings(Config config);

    cv::Mat erode(int erosion_dst, int erosion_size);

    cv::Mat dilate(int dilation_dst, int dilation_size);

    void findObjects();

    void pruneMasks();

   private:
    int directions[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};

    struct PointDirs {
        cv::Point coordinates;
        std::vector<cv::Point> directions;
    };

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
            return {cv::Point(-100, -100)};  // TODO return error
        }
    };

    bool walk(cv::Mat &output, uchar prev_z, double &mediumVal, int x, int y,
              uchar &id, int &visited, int &amount);

    void paint(cv::Point start, cv::Mat &output, uchar &id, Stats stats);

    void iterate(cv::Point start, cv::Mat &output, int imageLeft, uchar &id,
                 Stats stats);
};

#endif