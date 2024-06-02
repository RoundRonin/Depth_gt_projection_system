#ifndef TEMPLATEGEN_HPP
#define TEMPLATEGEN_HPP

#include <vector>

#include "opencv2/opencv.hpp"
#include "settings.hpp"

class Templates {
    int chessboard_size = 20;  // Size of each square in the chessboard
    int filler = 2 * chessboard_size;

    int m_iteration;

   public:
   public:
    Templates();

    void applyTemplates(std::vector<Template> template_list,
                        std::vector<cv::Mat> mask_mats, cv::Mat &image) {
        for (auto temp : template_list) {
            for (auto object_number : temp.objects) {
                try {
                    cv::Mat result(image.size(), image.type());
                    auto mask = mask_mats.at(object_number);
                    useTemplate(temp, mask, result);
                    cv::add(image, result, image);
                } catch (const std::exception &e) {
                    std::cerr << e.what() << '\n';
                }
            }
        }

        m_iteration++;
    }

    cv::Mat chessBoard(int iter, cv::Mat mask, int speedX = 1, int speedY = 1);

   private:
    cv::Mat gradient(int iter, cv::Mat mask, int a);

    cv::Mat solidColor(cv::Mat mask, cv::Scalar color);

    void useTemplate(Template temp, cv::Mat &mask, cv::Mat &out) {
        switch (temp.name) {
            case Template::GRAD:
                out = gradient(m_iteration, mask, temp.param_a);
                break;
            case Template::CHESS:
                out = chessBoard(m_iteration, mask, temp.speed_x, temp.speed_y);
                break;
            case Template::SOLID:
                out = solidColor(mask,
                                 {(double)temp.param_a, (double)temp.param_b,
                                  (double)temp.param_c});
                break;

            default:
                break;
        }
    }
};

#endif