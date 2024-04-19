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
    objects = Mat(image.size(), image.type());
    out_path = output_location; // TODO add checks
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

void Image::findObjects(uchar z_limit, int minDots, int maxObjects) {
    cerr << "[INFO] using z_limit = " << (int)z_limit << endl;
    cerr << "[INFO] using minDots = " << minDots << endl;
    cerr << "[INFO] using maxObjects = " << maxObjects << endl;

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
            if (val == 0)
                continue;
            if (objects.at<uchar>(i, j) != 0)
                continue;

            //! TIME
            auto recurse_start = chrono::high_resolution_clock::now();

            id = colors.at(mask_mats.size());
            Point current(j, i); // x, y //!
            Mat output =
                paint(image, objects, z_limit, current, id, visited); //!
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
    int version = 3;
    int batch = 5;
    string log_name = "recurse_log";
    log_system_stats(duration, "overall", version, batch, log_name);
    int iter = 0;
    for (auto duration : figure_durations) {
        log_system_stats(duration, "figure " + to_string(iter), version, batch,
                         log_name);
        iter++;
    }

    for (int i = 0; i < mask_mats.size(); i++) {
        imwrite(out_path + "mask " + to_string(i) + " .png", mask_mats.at(i));
    }

    imwrite(out_path + "objects.png", objects);
}

bool Image::walk(Mat &image, Mat &objects, Mat &output, uchar z_limit,
                 uchar prev_z, Point current, vector<Point> path, uchar id,
                 int &visited) {

    visited++;
    if (current.x > image.cols || current.y > image.rows)
        return false;

    if (objects.at<uchar>(current) != 0)
        return false;

    if (image.at<uchar>(current) == 0)
        return false;

    if (abs(image.at<uchar>(current) - prev_z) > z_limit)
        return false;

    objects.at<uchar>(current) = id; // Painting the pixel
    output.at<uchar>(current) = id;  // Painting the pixel

    path.push_back(current);

    // recurse

    for (int i = 0; i < 4; i++) {
        walk(image, objects, output, z_limit, image.at<uchar>(current),
             Point(current.x + directions[i][0], current.y + directions[i][1]),
             path, id, visited);
    }

    if (path.size() == 0)
        return false;
    path.pop_back();

    return false;
}

Mat Image::paint(Mat &image, Mat &objects, int z_limit, Point start, uchar id,
                 int &visited) {
    // Mat output = Mat::zeros(image.rows, image.cols, CV_8UC3);
    // Mat output = Mat(image.size(), image.type());
    Mat output = Mat(image.rows, image.cols, CV_8U, double(0));
    vector<Point> path;
    uchar start_z = image.at<uchar>(start);

    walk(image, objects, output, z_limit, start_z, start, path, id, visited);

    return output;
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