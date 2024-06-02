#ifndef TEMPLATEGEN_HPP
#define TEMPLATEGEN_HPP

#include <vector>

#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/opencv.hpp"

class Templates {
    int chessboard_size = 20;  // Size of each square in the chessboard
    int filler = 2 * chessboard_size;
    // int speedX = 1;
    // int speedY = 1;

    // int width = 1280;
    // int height = 720;
    std::vector<std::string> m_template_name_strings{"GRAD", "CHESS", "SOLID"};

   public:
    enum TemplateNames { GRAD, CHESS, SOLID };

   public:
    Templates();

    void applyTemplates(vector <) {}

    TemplateNames getTemplate(std::string name) {
        for (int i = 0; i < m_template_name_strings.size(); i++) {
            if (m_template_name_strings.at(i) == name)
                return static_cast<TemplateNames>(i);
        }

        throw std::runtime_error("No template with such name");
    }

    std::string to_string(TemplateNames temp) {
        return m_template_name_strings.at(static_cast<int>(temp));
    }

    void setResolution(cv::Size resolution);

    cv::Mat gradient(int iter, cv::Mat mask, int a);

    cv::Mat chessBoard(int iter, cv::Mat mask, int speedX = 1, int speedY = 1);

    cv::Mat solidColor(cv::Mat mask, cv::Scalar color);
};

#endif