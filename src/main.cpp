

#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/opencv.hpp"
#include <fstream>
#include <vector>

using namespace cv;
using namespace std;

class Image {
    cv::Mat objects;

  public:
    cv::Mat image;
    vector<cv::Mat> mask_mats;

  public:
    Image(string path) {
        image = cv::imread(path, IMREAD_GRAYSCALE);
        objects = cv::Mat(image.size(), image.type());
    }

    void write(string path) { cv::imwrite(path, image); }

    cv::Mat erode(int erosion_dst, int erosion_size) {
        int erosion_type = MORPH_ELLIPSE;
        Mat element = getStructuringElement(
            erosion_type, Size(2 * erosion_size + 1, 2 * erosion_size + 1),
            Point(erosion_size, erosion_size));

        cv::Mat output;
        cv::erode(image, output, element);

        image = output;
        return output;
    }

    cv::Mat dilate(int dilation_dst, int dilation_size) {
        int dilation_type = MORPH_ELLIPSE;
        Mat element = getStructuringElement(
            dilation_type, Size(2 * dilation_size + 1, 2 * dilation_size + 1),
            Point(dilation_size, dilation_size));

        cv::Mat output;
        cv::dilate(image, output, element);

        image = output;
        return output;
    }

    void findObjects(uchar z_limit = 10) {
        cout << "using z_limit = " << (int)z_limit << endl;
        if (image.empty())
            return;

        cout << "Stage 1" << endl;
        CV_Assert(image.depth() == CV_8U);
        int nRows = image.rows;
        int nCols = image.cols;
        uchar id = UCHAR_MAX;
        int minDots = 1000;
        int maxObjects = 5;

        vector<tuple<cv::Mat, int>> masks;

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
                // try {
                //     /* code */
                //     int dots = countNonZero(output);
                //     if (dots >= minDots)
                //         masks.push_back({output, dots});
                // } catch (const std::exception &e) {
                //     std::cerr << e.what() << '\n';
                // }
            }
        }

        // std::sort(masks.begin(), masks.end(),
        //           [](auto const &t1, auto const &t2) {
        //               return get<1>(t1) > get<1>(t2);
        //           });

        // for (auto var : masks) {
        //     cout << get<1>(var);
        //     cout << endl;
        // }

        for (int i = 0; i < maxObjects; i++) {
            // mask_mats.push_back(get<0>(masks.at(i)));
            // imwrite("mask " + to_string(i) + " .png", get<0>(masks.at(i)));
            cv::Mat masked;
            cv::bitwise_and(mask_mats.at(i), mask_mats.at(i), masked, objects);

            // CLEAN UP!! TODO remove
            int erosion_size = 3;

            int erosion_type = MORPH_ELLIPSE;
            Mat element = getStructuringElement(
                erosion_type, Size(2 * erosion_size + 1, 2 * erosion_size + 1),
                Point(erosion_size, erosion_size));

            cv::Mat output;
            cv::erode(masked, output, element);
            cv::dilate(output, output, element);
            mask_mats.at(i) = output;

            imwrite("mask " + to_string(i) + " .png", mask_mats.at(i));
        }

        imwrite("objects.png", objects);
    }

  private:
    int directions[4][2] = {{1, 0}, {0, 1}, {0, -1}, {-1, 0}};

    bool walk(cv::Mat &image, cv::Mat &objects, cv::Mat &output, uchar z_limit,
              uchar prev_z, cv::Point current, vector<cv::Point> path,
              uchar id) {
        // Base case:
        // 1. Pixel is off the map
        // cout << "new: ";
        // cout << "1 ";
        if (current.x > image.cols || current.y > image.rows)
            return false;

        // cout << "2 " << image.at<uchar>(current);
        // 2. Pixel is black
        if (image.at<uchar>(current) == 0)
            return false;

        // cout << "3 ";
        // 3. Pixel is in the objects
        if (objects.at<uchar>(current) != 0)
            return false;

        // cout << "4 ";
        // 4. z_limit exceeded
        if (abs(image.at<uchar>(current) - prev_z) > z_limit)
            return false;
        // cout << endl;

        // cout << current.x << " " << current.y << endl;
        objects.at<uchar>(current) = id; // Painting the pixel
        output.at<uchar>(current) = id;  // Painting the pixel

        // cout << "pushing..." << endl;
        path.push_back(current);
        // recurse
        // cout << "recursing..." << endl;
        for (int i = 0; i < 4; i++) {
            int x = directions[i][0];
            int y = directions[i][1];

            // cout << endl << i << ": " << x << " " << y << endl;
            cv::Point next(current.x + x, current.y + y);
            uchar current_z = image.at<uchar>(current);
            // cout << "..." << endl;
            walk(image, objects, output, z_limit, current_z, next, path, id);
        }

        // cout << "popping..." << endl;
        if (path.size() == 0)
            return false;
        path.pop_back();

        return false;
    }

    cv::Mat paint(cv::Mat &image, cv::Mat &objects, int z_limit,
                  cv::Point start, uchar id) {
        // cv::Mat output = cv::Mat::zeros(image.rows, image.cols, CV_8UC3);
        cv::Mat output = cv::Mat(image.size(), image.type());
        vector<cv::Point> path;
        uchar start_z = image.at<uchar>(start);

        walk(image, objects, output, z_limit, start_z, start, path, id);

        return output;
    }
};

class Templates {

    int chessboardSize = 20; // Size of each square in the chessboard
    int filler = 2 * chessboardSize;
    // int speedX = 1;
    // int speedY = 1;

    int width = 1280;
    int height = 720;

  public:
    Templates(cv::Mat mask) {
        width = mask.cols;
        height = mask.rows;
    }

    cv::Mat gradient(int iter, cv::Mat mask, int a) {
        Mat frame = Mat::zeros(height, width, CV_8UC3);
        uchar r = 255 * iter * a / (10 * 60);
        uchar g = 255 * (10 * 60 - iter * a) / (10 * 60);
        uchar b = 0;

        if (iter >= 5 * 60) {
            r = 0 + a * iter * 10;
            b = 255 * (iter * a - 5 * 60) / (5 * 60);
        }

        rectangle(frame, Point(0, 0), Point(width, height), Scalar(b, g, r),
                  -1);

        cv::Mat masked;
        cv::bitwise_and(frame, frame, masked, mask);

        return masked;
    }

    cv::Mat chessBoard(int iter, cv::Mat mask, int speedX = 1, int speedY = 1) {
        Mat frame = Mat::zeros(height, width, CV_8UC3);

        int offsetX = -(filler) + iter * speedX % (filler);

        int offsetY = -(filler) + iter * speedY % (filler);

        for (int y = 0; y < height + filler; y += chessboardSize) {
            for (int x = 0; x < width + filler; x += chessboardSize) {
                if (x / chessboardSize % 2 == (y / chessboardSize) % 2) {
                    rectangle(frame, Point(x + offsetX, y + offsetY),
                              Point(x + chessboardSize + offsetX,
                                    y + chessboardSize + offsetY),
                              Scalar(255, 255, 255), -1);
                } else {
                    rectangle(frame, Point(x + offsetX, y + offsetY),
                              Point(x + chessboardSize + offsetX,
                                    y + chessboardSize + offsetY),
                              Scalar(0, 0, 0), -1);
                }
            }
        }

        cv::Mat masked;
        cv::bitwise_and(frame, frame, masked, mask);

        return masked;
    }
};

int main(int argc, char **argv) {

    if (argc <= 1) {
        cout << "Usage: \n";
        cout << "$ " << argv[0] << " <DEPTH_MAP> \n";
        cout << "  ** Depth map file is mandatory in the application ** \n\n";
        return EXIT_FAILURE;
    }

    Image image(argv[1]);

    image.erode(5, 5);
    image.dilate(5, 5);

    image.erode(3, 3);
    image.dilate(3, 3);

    image.erode(2, 2);
    image.dilate(2, 2);

    image.dilate(3, 3);

    image.write("e_d_e_d_e_d_d.png");
    image.findObjects(atoi(argv[2]));

    int width = image.image.cols;
    int height = image.image.rows;
    cv::Size size = image.image.size();

    VideoWriter video("color_gradient.avi",
                      VideoWriter::fourcc('M', 'J', 'P', 'G'), 60, size);

    if (!video.isOpened()) {
        return -1;
    }

    vector<cv::Mat> masks = image.mask_mats;
    Templates templates(image.image);
    for (int i = 0; i < 10 * 60; i++) { // 10 seconds at 60 fps

        vector<cv::Mat> results;
        results.push_back(templates.chessBoard(i, masks.at(1), 2, 1));
        results.push_back(templates.chessBoard(i, masks.at(2), 1, 3));
        results.push_back(templates.gradient(i, masks.at(3), 1));
        results.push_back(templates.gradient(i, masks.at(4), 5));

        cv::add(results.at(0), results.at(1), results.at(1));
        cv::add(results.at(1), results.at(2), results.at(2));
        cv::add(results.at(2), results.at(3), results.at(3));

        video.write(results.at(3));
    }

    video.release();

    return EXIT_SUCCESS;
}
