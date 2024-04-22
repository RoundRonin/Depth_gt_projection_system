#include "object_recognition.hpp"
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace std;
using namespace cv;
namespace fs = std::filesystem;

Image::Image(string path, string output_location) {
    image = imread(path, IMREAD_GRAYSCALE);
    m_objects = Mat(image.rows, image.cols, CV_8U, double(0));
    // m_objects = Mat(image.size(), image.type());
    m_out_path = output_location; // TODO add checks
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

void Image::findObjects(uchar zlimit, int minDots, int maxObjects) {
    cerr << "[INFO] using zlimit = " << (int)zlimit << endl;
    cerr << "[INFO] using minDots = " << minDots << endl;
    cerr << "[INFO] using maxObjects = " << maxObjects << endl;

    m_zlimit = zlimit;

    if (image.empty())
        return;

    CV_Assert(image.depth() == CV_8U);
    int nRows = image.rows;
    int nCols = image.cols;
    uchar id = UCHAR_MAX;

    // vector<tuple<Mat, int>> masks;

    array<int, 10> colors = {30, 50, 70, 90, 110, 130, 150, 180, 210, 240};

    //! TIME
    auto start = chrono::high_resolution_clock::now();
    vector<chrono::microseconds> figure_durations;

    int visited = 0;
    int a = 0;
    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols; j++) {
            uchar val = image.at<uchar>(i, j);

            visited++;
            if (val == 0) // TODO min distance
                continue;
            if (m_objects.at<uchar>(i, j) != 0)
                continue;

            //! TIME
            auto recurse_start = chrono::high_resolution_clock::now();

            int amount = 0;
            // id = colors.at(mask_mats.size()); //!
            Point current(j, i); // x, y
            Mat output = paint(current, id, visited, amount);
            if (amount < minDots) {
                cerr << "[WARN] Area too smol (" << amount << "/" << minDots
                     << ")" << endl;
                continue;
            }

            if (mask_mats.size() < maxObjects)
                mask_mats.push_back(output);
            else
                cerr << "[WARN] Object limit exceeded (" << maxObjects << ")"
                     << endl;

            //! TIME
            auto recurse_stop = chrono::high_resolution_clock::now();
            auto recurse_duration = chrono::duration_cast<chrono::microseconds>(
                recurse_stop - recurse_start);
            figure_durations.push_back(recurse_duration);
        }
    }

    std::cout << "visited: " << visited << std::endl;
    std::cout << "total: " << image.rows * image.cols << std::endl;

    //! TIME
    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);

    // 10 gb
    int version = 4;
    int batch = 4;
    string log_name = "recurse_log";
    log_system_stats(duration, "overall", version, batch, log_name);
    int iter = 0;
    // for (auto duration : figure_durations) {
    //     log_system_stats(duration, "figure " + to_string(iter), version,
    //     batch,
    //                      log_name);
    //     iter++;
    // }

    for (int i = 0; i < mask_mats.size(); i++) {
        imwrite(m_out_path + "mask " + to_string(i) + " .png", mask_mats.at(i));
    }

    imwrite(m_out_path + "objects.png", m_objects);
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

Mat Image::paint(Point start, uchar &id, int &visited, int &amount) {
    // Mat output = Mat::zeros(image.rows, image.cols, CV_8UC3);
    // Mat output = Mat(image.size(), image.type());
    Mat output = Mat(image.rows, image.cols, CV_8U, double(0));
    uchar start_z = image.at<uchar>(start);

    walk(output, start_z, start.x, start.y, id, visited, amount);

    return output;
}

void Image::findObjectsIterative(uchar zlimit, int minDots, int maxObjects) {
    CV_Assert(image.depth() == CV_8U);
    if (image.empty())
        return;

    cerr << "[INFO] using zlimit = " << (int)zlimit << endl;
    cerr << "[INFO] using minDots = " << minDots << endl;
    cerr << "[INFO] using maxObjects = " << maxObjects << endl;

    m_zlimit = zlimit;

    // Image info
    int nRows = image.rows;
    int nCols = image.cols;
    int size = nCols * nRows;

    // Preinit
    int imageLeft = size;
    int visited = 0;
    int amount = 0;
    uchar id = UCHAR_MAX;

    // Preinit
    Mat output = Mat(image.rows, image.cols, CV_8U, double(0));

    Point current = Point(0, 0);
    uchar val = 0;

    auto start = chrono::high_resolution_clock::now();
    vector<chrono::microseconds> figure_durations;

    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols; j++) {
            // Skip undesired points
            current = Point(j, i);
            val = image.at<uchar>(current);
            visited++;
            if (val == 0) // TODO min distance
                continue;
            if (m_objects.at<uchar>(current) != 0)
                continue;

            //! TIME
            auto seek_start = chrono::high_resolution_clock::now();

            amount = 0;
            imageLeft = size - i * nCols + j;
            iterate(current, output, imageLeft, id, visited, amount);

            if (amount < minDots) {
                cerr << "[WARN] Area too smol (" << amount << "/" << minDots
                     << ")" << endl;
                continue;
            }

            if (mask_mats.size() < maxObjects)
                mask_mats.push_back(output);
            else
                cerr << "[WARN] Object limit exceeded (" << maxObjects << ")"
                     << endl;

            //! TIME
            auto seek_stop = chrono::high_resolution_clock::now();
            auto seek_duration = chrono::duration_cast<chrono::microseconds>(
                seek_stop - seek_start);
            figure_durations.push_back(seek_duration);
        }
    }

    std::cout << "visited: " << visited << std::endl;
    std::cout << "total: " << image.rows * image.cols << std::endl;

    //! TIME
    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);

    // 10 gb
    int version = 4;
    int batch = 4;
    string log_name = "iter_log";
    log_system_stats(duration, "overall", version, batch, log_name);
    int iter = 0;
    // for (auto duration : figure_durations) {
    //     log_system_stats(duration, "figure " + to_string(iter), version,
    //     batch,
    //                      log_name);
    //     iter++;
    // }

    for (int i = 0; i < mask_mats.size(); i++) {
        imwrite(m_out_path + "mask " + to_string(i) + " .png", mask_mats.at(i));
    }

    imwrite(m_out_path + "objects.png", m_objects);
}

void Image::iterate(Point start, Mat &output, int imageLeft, uchar &id,
                    int &visited, int &amount) {
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

enum time { second = 1, ms = 1000, mcs = 1000000 };

void Image::printTimeTaken(chrono::microseconds time_taken,
                           string function_name) {
    cout << "Time taken by " + function_name + " is: " << fixed
         << chrono::duration<double, ratio<1, time::second>>(time_taken).count()
         << setprecision(5);
    cout << " sec " << endl;
}

void Image::log_system_stats(chrono::microseconds time_taken,
                             string function_name, int version, int batch,
                             string log_name) {
    time_t now = chrono::system_clock::to_time_t(chrono::system_clock::now());

    string log_line =
        "{\"version\": " + to_string(version) +
        ", \"batch\": " + to_string(batch) + ", \"now\": " + to_string(now) +
        ", \"taken\": " +
        to_string(chrono::duration<double, ratio<1, time::second>>(time_taken)
                      .count()) +
        ", \"function\": \"" + function_name + "\"}";
    log_line.push_back(']');

    string file_path = "logs/" + log_name + ".json";
    if (!fs::exists(file_path)) {
        ofstream file(file_path);
        file << "[";
    }

    fstream file(file_path, ios::in | ios::out | ios::ate);
    if (!file.is_open()) {
        cerr << "Failed to open log file" << endl;
        return;
    }

    streampos file_len = file.tellg();

    if (file_len > 1) {
        file.seekp(file_len - 2);
        char last_byte;
        file >> last_byte;

        if (last_byte == ']') {
            file.seekp(file_len - 2);
            file << ",";
        } else {
            file.seekp(file_len - 1);
            file << ",";
        }
        file << endl;
    }

    // Write the log line
    file << log_line << endl;
}