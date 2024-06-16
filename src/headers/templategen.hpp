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

    // void applyTemplates(std::vector<TemplateDescription> template_list,
    //                     std::vector<optional<cv::Mat>> mask_mats,
    //                     cv::Mat &image) {
    //     for (auto temp : template_list) {
    //         for (auto object_number : temp.objects) {
    //             try {
    //                 if (object_number >= mask_mats.size()) continue;
    //                 cv::Mat result(image.size(), image.type());
    //                 if (mask_mats.at(object_number)) {
    //                     auto mask = *mask_mats.at(object_number);
    //                     useTemplate(temp, mask, result);

    //                     // std::cout << "IMAGE size, type, channels: " <<
    //                     // image.size()
    //                     //           << " " << image.type() << " " <<
    //                     //           image.channels()
    //                     //           << '\n';
    //                     // std::cout
    //                     //     << "RESULT size, type, channels: " <<
    //                     //     result.size()
    //                     //     << " " << result.type() << " " <<
    //                     //     result.channels()
    //                     //     << '\n';
    //                     CV_Assert(image.size() == result.size());
    //                     CV_Assert(image.type() == result.type());
    //                     CV_Assert(image.channels() == result.channels());
    //                     cv::add(image, result, image);
    //                 } else
    //                     std::cout << "Object gone! (" << object_number <<
    //                     ")\n";
    //             } catch (const std::exception &e) {
    //                 std::cerr << e.what() << '\n';
    //             }
    //         }
    //     }

    //     m_iteration++;
    // }

    void applyTemplates(std::vector<TemplateDescription> template_list,
                        std::vector<cv::Mat> mask_mats, cv::Mat &image) {
        for (auto temp : template_list) {
            for (auto object_number : temp.objects) {
                try {
                    if (object_number >= mask_mats.size()) continue;
                    cv::Mat result(image.size(), image.type());
                    auto mask = mask_mats.at(object_number);
                    useTemplate(temp, mask, result);

                    // std::cout << "IMAGE size, type, channels: " <<
                    // image.size()
                    //           << " " << image.type() << " " <<
                    //           image.channels()
                    //           << '\n';
                    // std::cout
                    //     << "RESULT size, type, channels: " << result.size()
                    //     << " " << result.type() << " " << result.channels()
                    //     << '\n';
                    CV_Assert(image.size() == result.size());
                    CV_Assert(image.type() == result.type());
                    CV_Assert(image.channels() == result.channels());
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

    cv::Mat horizontalLines(int iter, cv::Mat mask, int scale, int speed_x,
                            int speed_y, int speed_mulitplier,
                            cv::Scalar color);
    cv::Mat verticalLines(int iter, cv::Mat mask, int scale, int speed_x,
                          int speed_y, int speed_mulitplier, cv::Scalar color);
    cv::Mat circles(int iter, cv::Mat mask, int scale, int speed_x, int speed_y,
                    int speed_mulitplier, cv::Scalar color);

    void useTemplate(TemplateDescription temp, cv::Mat &mask, cv::Mat &out) {
        switch (temp.name) {
            case TemplateDescription::GRAD:
                out = gradient(m_iteration, mask, temp.param_a);
                break;
            case TemplateDescription::CHESS:
                out = chessBoard(m_iteration, mask, temp.speed_x, temp.speed_y);
                break;
            case TemplateDescription::SOLID:
                out = solidColor(mask,
                                 {(double)temp.param_a, (double)temp.param_b,
                                  (double)temp.param_c});
                break;
            case TemplateDescription::VERTL:
                out = verticalLines(m_iteration, mask, temp.scale, temp.speed_x,
                                    temp.speed_y, temp.speed_multiplier,
                                    {(double)temp.param_a, (double)temp.param_b,
                                     (double)temp.param_c});
                break;
            case TemplateDescription::HORL:
                out =
                    horizontalLines(m_iteration, mask, temp.scale, temp.speed_x,
                                    temp.speed_y, temp.speed_multiplier,
                                    {(double)temp.param_a, (double)temp.param_b,
                                     (double)temp.param_c});
                break;
            case TemplateDescription::CIRCLES:
                out = circles(m_iteration, mask, temp.scale, temp.speed_x,
                              temp.speed_y, temp.speed_multiplier,
                              {(double)temp.param_a, (double)temp.param_b,
                               (double)temp.param_c});
                break;

            default:
                break;
        }
    }
};

#endif