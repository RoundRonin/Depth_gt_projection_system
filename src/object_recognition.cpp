#include "object_recognition.hpp"

Image::Image(std::string path, std::string output_location) {
    image = cv::imread(path, cv::IMREAD_GRAYSCALE);
    objects = cv::Mat(image.size(), image.type());
    out_path = output_location; // TODO add checks
}

void Image::write(std::string path) { cv::imwrite(path, image); }

cv::Mat Image::erode(int erosion_dst, int erosion_size) {
    int erosion_type = cv::MORPH_ELLIPSE;
    cv::Mat element = cv::getStructuringElement(
        erosion_type, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1),
        cv::Point(erosion_size, erosion_size));

    cv::Mat output;
    cv::erode(image, output, element);

    image = output;
    return output;
}

cv::Mat Image::dilate(int dilation_dst, int dilation_size) {
    int dilation_type = cv::MORPH_ELLIPSE;
    cv::Mat element = cv::getStructuringElement(
        dilation_type, cv::Size(2 * dilation_size + 1, 2 * dilation_size + 1),
        cv::Point(dilation_size, dilation_size));

    cv::Mat output;
    cv::dilate(image, output, element);

    image = output;
    return output;
}

void Image::findObjects(uchar z_limit) {
    std::cout << "using z_limit = " << (int)z_limit << std::endl;
    if (image.empty())
        return;

    std::cout << "Stage 1" << std::endl;
    CV_Assert(image.depth() == CV_8U);
    int nRows = image.rows;
    int nCols = image.cols;
    uchar id = UCHAR_MAX;
    int minDots = 1000;
    int maxObjects = 5;

    std::vector<std::tuple<cv::Mat, int>> masks;

    int a = 0;
    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols; j++) {
            uchar val = image.at<uchar>(i, j);

            if (val == 0)
                continue;
            if (objects.at<uchar>(i, j) != 0)
                continue;

            cv::Point current(j, i); // x, y
            cv::Mat output = paint(image, objects, z_limit, current, id);
            mask_mats.push_back(output);
        }
    }

    for (int i = 0; i < maxObjects; i++) {
        cv::Mat masked;
        cv::bitwise_and(mask_mats.at(i), mask_mats.at(i), masked, objects);

        // CLEAN UP!! TODO remove
        int erosion_size = 3;

        int erosion_type = cv::MORPH_ELLIPSE;
        cv::Mat element = cv::getStructuringElement(
            erosion_type, cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1),
            cv::Point(erosion_size, erosion_size));

        cv::Mat output;
        cv::erode(masked, output, element);
        cv::dilate(output, output, element);
        mask_mats.at(i) = output;

        cv::imwrite(out_path + "mask " + std::to_string(i) + " .png",
                    mask_mats.at(i));
    }

    cv::imwrite(out_path + "objects.png", objects);
}

bool Image::walk(cv::Mat &image, cv::Mat &objects, cv::Mat &output,
                 uchar z_limit, uchar prev_z, cv::Point current,
                 std::vector<cv::Point> path, uchar id) {

    if (current.x > image.cols || current.y > image.rows)
        return false;

    if (image.at<uchar>(current) == 0)
        return false;

    if (objects.at<uchar>(current) != 0)
        return false;

    if (abs(image.at<uchar>(current) - prev_z) > z_limit)
        return false;

    objects.at<uchar>(current) = id; // Painting the pixel
    output.at<uchar>(current) = id;  // Painting the pixel

    path.push_back(current);

    // recurse

    for (int i = 0; i < 4; i++) {
        int x = directions[i][0];
        int y = directions[i][1];

        cv::Point next(current.x + x, current.y + y);
        uchar current_z = image.at<uchar>(current);

        walk(image, objects, output, z_limit, current_z, next, path, id);
    }

    if (path.size() == 0)
        return false;
    path.pop_back();

    return false;
}

cv::Mat Image::paint(cv::Mat &image, cv::Mat &objects, int z_limit,
                     cv::Point start, uchar id) {
    // cv::Mat output = cv::Mat::zeros(image.rows, image.cols, CV_8UC3);
    cv::Mat output = cv::Mat(image.size(), image.type());
    std::vector<cv::Point> path;
    uchar start_z = image.at<uchar>(start);

    walk(image, objects, output, z_limit, start_z, start, path, id);

    return output;
}