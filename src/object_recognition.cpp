#include "object_recognition.hpp"
#include "utils.hpp"

using namespace std;
using namespace cv;
namespace fs = std::filesystem;

Image::Image(string path, string output_location, const Logger &log) {
    image = imread(path, IMREAD_GRAYSCALE);
    CV_Assert(image.depth() == CV_8U);
    if (image.empty())
        return;

    m_objects = Mat(image.rows, image.cols, CV_8U, double(0));
    m_out_path = output_location; // TODO add checks

    m_log = log;
}

void Image::write(string path) { imwrite(path, image); }

Mat Image::erode(int erosion_dst, int erosion_size) {
    int erosion_type = MORPH_ELLIPSE;
    Mat element = getStructuringElement(
        erosion_type, Size(2 * erosion_size + 1, 2 * erosion_size + 1),
        Point(erosion_size, erosion_size));

    Mat output;
    cv::erode(image, output, element);

    image = output;
    return output;
}

Mat Image::dilate(int dilation_dst, int dilation_size) {
    int dilation_type = MORPH_ELLIPSE;
    Mat element = getStructuringElement(
        dilation_type, Size(2 * dilation_size + 1, 2 * dilation_size + 1),
        Point(dilation_size, dilation_size));

    Mat output;
    cv::dilate(image, output, element);

    image = output;
    return output;
}

void Image::findObjects(uchar zlimit, uchar minDistance, int minDots,
                        int maxObjects, bool recurse) {

    // printFindInfo(zlimit, minDistance, minDots, maxObjects);
    auto i_use = Logger::ERROR::INFO_USING;
    auto i_info = Logger::ERROR::INFO;
    auto w_smol = Logger::ERROR::WARN_SMOL_AREA;
    auto w_limit = Logger::ERROR::WARN_OBJECT_LIMIT;

    m_log.log_message({i_use, {(int)zlimit}, "depth difference limit"});
    m_log.log_message({i_use, {(int)minDistance}, "minimum distance"});
    m_log.log_message({i_use, {minDots}, "minimum area"});
    m_log.log_message({i_use, {maxObjects}, "maximum number of objects"});

    m_zlimit = zlimit;

    // Image info
    int nRows = image.rows;
    int nCols = image.cols;
    int size = nCols * nRows;

    // Preinit
    int imageLeft = size;
    int visited = 0;
    int amount = 0;
    int x, y = 0;
    Stats stats = {visited, amount};
    uchar id = UCHAR_MAX;

    // Preinit
    Mat output = Mat(image.rows, image.cols, CV_8U, double(0));
    Point current = Point(0, 0);
    uchar val = 0;

    m_log.start();

    for (int i = 0; i < nRows * nCols; i++) {
        x = i % nCols;
        y = i / nCols;
        // Skip undesired points
        current = Point(x, y);
        val = image.at<uchar>(current);
        visited++;
        if (val <= minDistance)
            continue;
        if (m_objects.at<uchar>(current) != 0)
            continue;

        m_log.start();
        amount = 0;
        imageLeft = size - y * nCols + x;
        if (recurse)
            paint(current, output, id, stats);
        else
            iterate(current, output, imageLeft, id, stats);

        if (amount < minDots) {
            m_log.log_message({w_smol, {amount, minDots}});
            m_log.drop();
            continue;
        }

        if (mask_mats.size() < maxObjects)
            mask_mats.push_back(output);
        else {
            m_log.log_message({w_limit, {maxObjects}});
            m_log.drop();
            continue;
        }

        m_log.stop("seek");
    }

    m_log.log_message({i_info, {visited}, "visited"});
    m_log.log_message({i_info, {image.rows * image.cols}, "total"});

    m_log.stop("overall");
    m_log.print();
    m_log.log();

    for (int i = 0; i < mask_mats.size(); i++) {
        imwrite(m_out_path + "mask " + to_string(i) + " .png", mask_mats.at(i));
    }

    imwrite(m_out_path + "objects.png", m_objects);
}

void Image::iterate(Point start, Mat &output, int imageLeft, uchar &id,
                    Stats stats) {
    auto [visited, amount] = stats;

    output = Mat(image.rows, image.cols, CV_8U, double(0));
    m_objects.at<uchar>(start) = id;
    output.at<uchar>(start) = id;

    uchar previous_z = 0;

    Point current_point = start;
    Point point_to_check = start;

    Dirs dirs;
    vector<Point> current_direction = dirs.toRIGHT;
    vector<Point> check_list{current_point};
    // vector<Point> next_direction = current_direction;

    for (int check = 0; check < check_list.size() && check < imageLeft * 4;
         check++) {
        current_point =
            check_list.at(check_list.size() - 1); // TODO is this optimal?
        check_list.pop_back();

        for (int iter = 0; iter < imageLeft; iter++) {
            previous_z = image.at<uchar>(current_point);
            // for (Point direction : current_direction) {
            //     check_list.push_back(point_to_check + direction);
            // }
            for (int r = 3; r > 0; r--) {
                check_list.push_back(point_to_check +
                                     current_direction.at(r - 1));
            }

            for (Point direction : current_direction) {
                check_list.pop_back();
                point_to_check = current_point;
                point_to_check += direction;

                visited++;
                // ALL THE CHECKS
                if (point_to_check.x >= image.cols ||
                    point_to_check.y >= image.rows || point_to_check.x < 0 ||
                    point_to_check.y < 0)
                    continue;
                if (m_objects.at<uchar>(point_to_check) != 0)
                    continue;
                if (image.at<uchar>(point_to_check) == 0)
                    continue;
                if (abs(image.at<uchar>(point_to_check) - previous_z) >
                    m_zlimit)
                    continue;

                amount++;
                current_point = point_to_check;
                current_direction = dirs.nextDirectionList(direction);
                break;
            }

            if (current_point != point_to_check)
                break;

            m_objects.at<uchar>(current_point) = id;
            output.at<uchar>(current_point) = id;
        }
    }
}

void Image::paint(Point start, Mat &output, uchar &id, Stats stats) {
    auto [visited, amount] = stats;

    output = Mat(image.rows, image.cols, CV_8U, double(0));
    uchar start_z = image.at<uchar>(start);

    walk(output, start_z, start.x, start.y, id, visited, amount);
}

bool Image::walk(Mat &output, uchar prev_z, int x, int y, uchar &id,
                 int &visited, int &amount) {

    visited++;
    if (x >= image.cols || y >= image.rows || x < 0 || y < 0)
        return false;

    Point current = Point(x, y);
    if (m_objects.at<uchar>(current) != 0)
        return false;

    if (image.at<uchar>(current) == 0)
        return false;

    if (abs(image.at<uchar>(current) - prev_z) > m_zlimit)
        return false;

    m_objects.at<uchar>(current) = id; // Painting the pixel
    output.at<uchar>(current) = id;    // Painting the pixel
    amount++;

    // recurse
    for (int i = 0; i < 4; i++) {
        walk(output, image.at<uchar>(current), current.x + directions[i][0],
             current.y + directions[i][1], id, visited, amount);
    }

    return false;
}