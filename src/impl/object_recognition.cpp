#include "../headers/object_recognition.hpp"

#include "../headers/utils.hpp"

using namespace std;
using namespace cv;
namespace fs = std::filesystem;

ImageProcessor::ImageProcessor(string output_location, const Logger &log,
                               const Printer &printer)
    : m_log(log), m_printer(printer) {
    m_out_path = output_location;  // TODO add checks
}

ImageProcessor::ImageProcessor(string path, string output_location,
                               const Logger &log, const Printer &printer)
    : m_log(log), m_printer(printer) {
    // TODO check if image exists
    image = imread(path, IMREAD_GRAYSCALE);
    CV_Assert(image.depth() == CV_8U);
    if (image.empty()) return;

    m_objects = cv::Mat(image.rows, image.cols, CV_8U, double(0));
    m_out_path = output_location;  // TODO add checks
}

ImageProcessor::ImageProcessor(cv::Mat o_image, std::string output_location,
                               const Logger &log, const Printer &printer)
    : m_log(log), m_printer(printer) {
    image = o_image;
    CV_Assert(image.depth() == CV_8U);
    if (image.empty()) return;

    m_objects = cv::Mat(image.rows, image.cols, CV_8U, double(0));
    m_out_path = output_location;  // TODO add checks
}

void ImageProcessor::getImage(string path) {
    // TODO add checks, return error
    image = imread(path, IMREAD_GRAYSCALE);
    CV_Assert(image.depth() == CV_8U);
    if (image.empty()) return;

    m_objects = cv::Mat(image.rows, image.cols, CV_8U, double(0));
}

void ImageProcessor::write(string path) {
    // TODO check if folder exists, create if it doesn't
    imwrite(path, image);
}

cv::Mat ImageProcessor::erode(int erosion_dst, int erosion_size) {
    int erosion_type = MORPH_ELLIPSE;
    cv::Mat element = getStructuringElement(
        erosion_type, Size(2 * erosion_size + 1, 2 * erosion_size + 1),
        Point(erosion_size, erosion_size));

    cv::Mat output;
    cv::erode(image, output, element);

    image = output;
    return output;
}

cv::Mat ImageProcessor::dilate(int dilation_dst, int dilation_size) {
    int dilation_type = MORPH_ELLIPSE;
    cv::Mat element = getStructuringElement(
        dilation_type, Size(2 * dilation_size + 1, 2 * dilation_size + 1),
        Point(dilation_size, dilation_size));

    cv::Mat output;
    cv::dilate(image, output, element);

    image = output;
    return output;
}

void ImageProcessor::findObjects(uchar zlimit, uchar minDistance,
                                 uchar medium_limit, int minDots,
                                 int maxObjects, bool recurse) {
    // printFindInfo(zlimit, minDistance, minDots, maxObjects);
    auto i_use = Printer::ERROR::INFO_USING;
    auto i_info = Printer::ERROR::INFO;
    auto w_smol = Printer::ERROR::WARN_SMOL_AREA;
    auto w_limit = Printer::ERROR::WARN_OBJECT_LIMIT;

    auto p = Printer::DEBUG_LVL::PRODUCTION;
    auto b = Printer::DEBUG_LVL::BRIEF;

    m_printer.log_message({i_use, {(int)zlimit}, "depth difference limit", p});
    m_printer.log_message({i_use, {(int)minDistance}, "min distance", p});
    m_printer.log_message({i_use, {(int)medium_limit}, "medium limit", p});
    m_printer.log_message({i_use, {minDots}, "min area", p});
    m_printer.log_message({i_use, {maxObjects}, "max number of objects", p});

    m_zlimit = zlimit;
    m_min_distance = minDistance;
    m_medium_limit = medium_limit;

    // ImageProcessor info
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
    cv::Mat output = cv::Mat(image.rows, image.cols, CV_8U, double(0));
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
        if (val <= m_min_distance) continue;
        if (m_objects.at<uchar>(current) != 0) continue;

        m_log.start();
        amount = 0;
        imageLeft = size - y * nCols + x;  // TODO
        if (recurse)
            paint(current, output, id, stats);
        else
            iterate(current, output, imageLeft, id, stats);

        if (amount < minDots) {
            m_printer.log_message({w_smol, {amount, minDots}});
            m_log.drop();
            continue;
        }

        if (mask_mats.size() < maxObjects)
            mask_mats.push_back(output);
        else {
            m_printer.log_message({w_limit, {maxObjects}});
            m_log.drop();
            continue;
        }

        m_log.stop("seek");
    }

    m_printer.log_message({i_info, {visited}, "visited", p});
    m_printer.log_message({i_info, {image.rows * image.cols}, "total", p});

    m_log.stop("overall");
    m_log.print();
    m_log.log();

    for (int i = 0; i < mask_mats.size(); i++) {
        imwrite(m_out_path + "mask " + to_string(i) + " .png", mask_mats.at(i));
    }

    imwrite(m_out_path + "objects.png", m_objects);
}

void ImageProcessor::iterate(Point start, cv::Mat &output, int imageLeft,
                             uchar &id, Stats stats) {
    auto [visited, amount] = stats;

    output = cv::Mat(image.rows, image.cols, CV_8U, double(0));
    m_objects.at<uchar>(start) = id;
    output.at<uchar>(start) = id;
    Dirs dirs;

    uchar previous_z = 0;
    Point point_to_check = start;
    PointDirs pd{point_to_check, dirs.toRIGHT};
    m_objects.at<uchar>(point_to_check) = id;
    output.at<uchar>(point_to_check) = id;

    vector<PointDirs> list{pd};

    double mediumVal = 0;

    int iter = 0;
    while (list.size() != 0 && iter < imageLeft * 4) {
        iter++;
        pd = list.back();
        list.pop_back();

        previous_z = image.at<uchar>(pd.coordinates);
        mediumVal = (mediumVal * amount + previous_z) / (amount + 1);
        for (auto dir : pd.directions) {
            point_to_check = pd.coordinates;
            point_to_check += dir;

            visited++;
            // Basic checks
            if (point_to_check.x >= image.cols ||
                point_to_check.y >= image.rows || point_to_check.x < 0 ||
                point_to_check.y < 0)
                continue;
            if (m_objects.at<uchar>(point_to_check) != 0) continue;
            if (image.at<uchar>(point_to_check) <= m_min_distance) continue;
            if (abs(image.at<uchar>(point_to_check) - previous_z) > m_zlimit)
                continue;

            // Additional checks
            // medium check
            if (abs(image.at<uchar>(point_to_check) - mediumVal) >
                m_medium_limit)
                continue;

            amount++;
            // TODO 1. Add area to objects later and do checks via output
            // TODO 2. Redo the object with a different set of constraints in
            // TODO certain conditions
            // TODO or not the whole object but chunk-by-chunk (!)
            m_objects.at<uchar>(point_to_check) = id;
            output.at<uchar>(point_to_check) = id;

            list.push_back({point_to_check, dirs.nextDirectionList(dir)});
        }
    }
}

void ImageProcessor::paint(Point start, cv::Mat &output, uchar &id,
                           Stats stats) {
    auto [visited, amount] = stats;

    output = cv::Mat(image.rows, image.cols, CV_8U, double(0));
    uchar start_z = image.at<uchar>(start);
    double mediumVal = start_z;

    walk(output, start_z, mediumVal, start.x, start.y, id, visited, amount);
}

bool ImageProcessor::walk(cv::Mat &output, uchar prev_z, double &mediumVal,
                          int x, int y, uchar &id, int &visited, int &amount) {
    visited++;
    // Basic checks
    if (x >= image.cols || y >= image.rows || x < 0 || y < 0) return false;
    Point current = Point(x, y);
    if (m_objects.at<uchar>(current) != 0) return false;
    if (image.at<uchar>(current) <= m_min_distance) return false;
    if (abs(image.at<uchar>(current) - prev_z) > m_zlimit) return false;

    // Additional checks
    // medium check
    if (abs(image.at<uchar>(current) - mediumVal) > m_medium_limit)
        return false;

    m_objects.at<uchar>(current) = id;  // Painting the pixel
    output.at<uchar>(current) = id;     // Painting the pixel
    amount++;
    mediumVal = (mediumVal * amount + prev_z) / (amount + 1);
    // recurse
    for (int i = 0; i < 4; i++) {
        walk(output, image.at<uchar>(current), mediumVal,
             current.x + directions[i][0], current.y + directions[i][1], id,
             visited, amount);
    }

    return false;
}