#include "../headers/templategen.hpp"

Templates::Templates() {}

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

    rectangle(frame, cv::Point(0, 0), cv::Point(mask.size()),
              cv::Scalar(b, g, r), -1);

    cv::Mat masked;
    cv::Size size = mask.size();

    CV_Assert(mask.size() == frame.size());

    cv::imwrite("./Result/temp_grad.png", frame);
    cv::bitwise_and(frame, frame, masked, mask);
    return masked;
}

cv::Mat Templates::chessBoard(int iter, cv::Mat mask, int speedX, int speedY) {
    // TODO FIX IT
    cv::Mat frame = cv::Mat::zeros(mask.size(), CV_8UC3);

    int offsetX = -(filler) + iter * speedX % (filler);

    int offsetY = -(filler) + iter * speedY % (filler);

    for (int y = 0; y < mask.size().height + filler; y += chessboard_size) {
        for (int x = 0; x < mask.size().width + filler; x += chessboard_size) {
            if (x / chessboard_size % 2 == (y / chessboard_size) % 2) {
                rectangle(frame, cv::Point(x + offsetX, y + offsetY),
                          cv::Point(x + chessboard_size + offsetX,
                                    y + chessboard_size + offsetY),
                          cv::Scalar(255, 255, 255), -1);
            } else {
                rectangle(frame, cv::Point(x + offsetX, y + offsetY),
                          cv::Point(x + chessboard_size + offsetX,
                                    y + chessboard_size + offsetY),
                          cv::Scalar(0, 0, 0), -1);
            }
        }
    }

    cv::imwrite("./Result/temp_chess.png", frame);
    cv::Mat masked;
    cv::bitwise_and(frame, frame, masked, mask);

    return masked;
}

cv::Mat Templates::solidColor(cv::Mat mask, cv::Scalar color) {
    cv::Mat frame = cv::Mat::zeros(mask.size(), CV_8UC3);

    rectangle(frame, cv::Point(0, 0), cv::Point(mask.size()), color, -1);

    cv::imwrite("./Result/temp_solid.png", frame);
    cv::Mat masked;
    cv::bitwise_and(frame, frame, masked, mask);

    return masked;
}

cv::Mat Templates::horizontalLines(int iter, cv::Mat mask, int scale,
                                   int speed_x, int speed_y,
                                   int speed_multiplier, cv::Scalar color) {
    cv::Mat frame = cv::Mat::zeros(mask.size(), CV_8UC3);

    int size = 20 * scale;

    int offset_y = -(filler) + iter * speed_y * speed_multiplier % (filler);

    for (int y = 0; y < mask.size().height + filler; y += size) {
        // for (int x = 0; x < mask.size().width + filler; x += chessboard_size)
        // {
        if (y / size % 2) {
            rectangle(frame, cv::Point(0, y + offset_y),
                      cv::Point(mask.size().width, y + size + offset_y), color,
                      -1);
        } else {
            rectangle(frame, cv::Point(0, y + offset_y),
                      cv::Point(mask.size().width, y + size + offset_y),
                      cv::Scalar(0, 0, 0), -1);
        }
        // }
    }

    cv::imwrite("./Result/temp_horizontalLines.png", frame);
    cv::Mat masked;
    cv::bitwise_and(frame, frame, masked, mask);
    return masked;
}
cv::Mat Templates::verticalLines(int iter, cv::Mat mask, int scale, int speed_x,
                                 int speed_y, int speed_multiplier,
                                 cv::Scalar color) {
    cv::Mat frame = cv::Mat::zeros(mask.size(), CV_8UC3);

    int size = 20 * scale;
    int offset_x = -(filler) + iter * speed_x * speed_multiplier % (filler);
    for (int x = 0; x < mask.size().width + filler; x += size) {
        if (x / size % 2) {
            rectangle(frame, cv::Point(x + offset_x, 0),
                      cv::Point(x + size + offset_x, mask.size().height), color,
                      -1);
        } else {
            rectangle(frame, cv::Point(x + offset_x, 0),
                      cv::Point(x + size + offset_x, mask.size().height),
                      cv::Scalar(0, 0, 0), -1);
        }
    }

    cv::imwrite("./Result/temp_verticalLines.png", frame);
    cv::Mat masked;
    cv::bitwise_and(frame, frame, masked, mask);
    return masked;
}
cv::Mat Templates::circles(int iter, cv::Mat mask, int scale, int speed_x,
                           int speed_y, int speed_multiplier,
                           cv::Scalar color) {
    cv::Mat frame = cv::Mat::zeros(mask.size(), CV_8UC3);

    // Calculate the center of the frame
    cv::Point center(mask.cols / 2, mask.rows / 2);

    int initial_radius = scale;
    int index = 0;

    bool colored = true;

    for (int i = iter * speed_multiplier; i > 0; i -= scale) {
        int radius = initial_radius + iter * speed_multiplier - i;
        cv::Scalar current_color = colored ? color : cv::Scalar{0, 0, 0};

        cv::circle(frame, center, radius, current_color, -1);
        colored = !colored;
    }

    // while (true)
    //     index += speed_multiplier;
    //     // Calculate the radius for each ring based on the iteration
    //     int radius = initial_radius + (iter - i * speed_x) *
    //     speed_multiplier;

    //     // Ensure the radius is positive
    //     if (radius > 0) {
    //         // Set the thickness of the ring
    //         // int thickness = scale / 2;
    //         if (radius % 10 == 0) color = {0, 0, 0};

    //         // Draw the ring
    //         cv::circle(frame, center, radius, color, -1);
    //     }
    // }

    cv::imwrite("./Result/temp_circles.png", frame);
    cv::Mat masked;
    cv::bitwise_and(frame, frame, masked, mask);
    return masked;
}