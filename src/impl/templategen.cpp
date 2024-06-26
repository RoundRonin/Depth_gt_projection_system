#include "../headers/templategen.hpp"

Templates::Templates(cv::Size resolution) {
    width = resolution.width;
    height = resolution.height;
}

void Templates::setResolution(cv::Size resolution) {
    width = resolution.width;
    height = resolution.height;
}

cv::Mat Templates::gradient(int iter, cv::Mat mask, int a) {
    CV_Assert(mask.type() == CV_8UC1);
    cv::Mat frame = cv::Mat::zeros(mask.size(), CV_8UC3);
    uchar r = 255 * iter * a / (10 * 60);
    uchar g = 255 * (10 * 60 - iter * a) / (10 * 60);
    uchar b = 0;

    if (iter >= 5 * 60) {
        r = 0 + a * iter * 10;
        b = 255 * (iter * a - 5 * 60) / (5 * 60);
    }

    rectangle(frame, cv::Point(0, 0), cv::Point(width, height),
              cv::Scalar(b, g, r), -1);

    cv::Mat masked;
    cv::Size size = mask.size();

    CV_Assert(mask.size() == frame.size());

    cv::bitwise_and(frame, frame, masked, mask);
    return masked;
}

cv::Mat Templates::chessBoard(int iter, cv::Mat mask, int speedX, int speedY) {
    // TODO FIX IT
    cv::Mat frame = cv::Mat::zeros(mask.size(), CV_8UC3);

    int offsetX = -(filler) + iter * speedX % (filler);

    int offsetY = -(filler) + iter * speedY % (filler);

    for (int y = 0; y < height + filler; y += chessboardSize) {
        for (int x = 0; x < width + filler; x += chessboardSize) {
            if (x / chessboardSize % 2 == (y / chessboardSize) % 2) {
                rectangle(frame, cv::Point(x + offsetX, y + offsetY),
                          cv::Point(x + chessboardSize + offsetX,
                                    y + chessboardSize + offsetY),
                          cv::Scalar(255, 255, 255), -1);
            } else {
                rectangle(frame, cv::Point(x + offsetX, y + offsetY),
                          cv::Point(x + chessboardSize + offsetX,
                                    y + chessboardSize + offsetY),
                          cv::Scalar(0, 0, 0), -1);
            }
        }
    }

    cv::Mat masked;
    cv::bitwise_and(frame, frame, masked, mask);

    return masked;
}

cv::Mat Templates::solidColor(cv::Mat mask, cv::Scalar color) {
    cv::Mat frame = cv::Mat::zeros(mask.size(), CV_8UC3);

    rectangle(frame, cv::Point(0, 0), cv::Point(width, height), color, -1);

    cv::Mat masked;
    cv::bitwise_and(frame, frame, masked, mask);

    return masked;
}