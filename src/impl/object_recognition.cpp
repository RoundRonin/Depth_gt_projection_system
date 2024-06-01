#include "../headers/object_recognition.hpp"

#include "../headers/utils.hpp"

using namespace std;
using namespace cv;
namespace fs = std::filesystem;

ImageProcessor::ImageProcessor(string output_location, Logger &log,
                               Printer &printer)
    : m_log(log), m_printer(printer) {
    m_out_path = output_location;  // TODO add checks
}

ImageProcessor::ImageProcessor(string path, string output_location, Logger &log,
                               Printer &printer)
    : m_log(log), m_printer(printer) {
    // TODO check if image exists
    (*image) = imread(path, IMREAD_GRAYSCALE);
    CV_Assert((*image).depth() == CV_8U);
    if ((*image).empty()) return;

    m_objects = cv::Mat((*image).rows, (*image).cols, CV_8U, double(0));
    m_out_path = output_location;  // TODO add checks
}

ImageProcessor::ImageProcessor(cv::Mat o_image, std::string output_location,
                               Logger &log, Printer &printer)
    : m_log(log), m_printer(printer) {
    // todo fix
    (*image) = o_image;
    CV_Assert((*image).depth() == CV_8U);
    if ((*image).empty()) return;

    m_objects = cv::Mat((*image).rows, (*image).cols, CV_8U, double(0));
    m_out_path = output_location;  // TODO add checks
}

void ImageProcessor::getImage(string path) {
    // TODO add checks, return error
    cv::Mat new_image = imread(path, IMREAD_GRAYSCALE);
    CV_Assert(new_image.depth() == CV_8U);
    if (new_image.empty()) return;
    (*image) = new_image;

    m_objects = cv::Mat((*image).rows, (*image).cols, CV_8U, double(0));
}

void ImageProcessor::getImage(cv::Mat *new_image) {
    // TODO add checks, return error
    CV_Assert((*new_image).depth() == CV_8U);
    CV_Assert((*new_image).channels() == 1);
    CV_Assert((*new_image).empty() == false);
    // (*image) = new_image;
    image = new_image;
    // if (image.empty()) return;

    m_objects = cv::Mat((*image).rows, (*image).cols, CV_8U, double(0));
}

void ImageProcessor::write(string path) {
    // TODO check if folder exists, create if it doesn't
    imwrite(path, (*image));
}

void ImageProcessor::setParameteresFromSettings(Settings settings) {
    // TODO cheks?
    m_parameters.z_limit = settings.config.z_limit;
    m_parameters.min_distance = settings.config.medium_limit;
    m_parameters.medium_limit = settings.config.medium_limit;
    m_parameters.min_area = settings.config.min_area;
    m_parameters.max_objects = settings.config.max_objects;
    m_parameters.recurse = settings.config.recurse;
}

cv::Mat ImageProcessor::erode(int erosion_dst, int erosion_size) {
    int erosion_type = MORPH_ELLIPSE;
    cv::Mat element = getStructuringElement(
        erosion_type, Size(2 * erosion_size + 1, 2 * erosion_size + 1),
        Point(erosion_size, erosion_size));

    cv::Mat output;
    cv::erode((*image), output, element);

    (*image) = output;
    return output;
}

cv::Mat ImageProcessor::dilate(int dilation_dst, int dilation_size) {
    int dilation_type = MORPH_ELLIPSE;
    cv::Mat element = getStructuringElement(
        dilation_type, Size(2 * dilation_size + 1, 2 * dilation_size + 1),
        Point(dilation_size, dilation_size));

    cv::Mat output;
    cv::dilate((*image), output, element);

    (*image) = output;
    return output;
}

void ImageProcessor::findObjects() {
    // printFindInfo(zlimit, minDistance, minDots, maxObjects);
    auto i_use = Printer::ERROR::INFO_USING;
    auto i_info = Printer::ERROR::INFO;
    auto w_smol = Printer::ERROR::WARN_SMOL_AREA;
    auto w_limit = Printer::ERROR::WARN_OBJECT_LIMIT;

    auto p = Printer::DEBUG_LVL::PRODUCTION;
    // auto b = Printer::DEBUG_LVL::BRIEF;

    m_printer.log_message(
        {i_use, {(int)m_parameters.z_limit}, "depth difference limit", p});
    m_printer.log_message(
        {i_use, {(int)m_parameters.min_distance}, "min distance", p});
    m_printer.log_message(
        {i_use, {(int)m_parameters.medium_limit}, "medium limit", p});
    m_printer.log_message({i_use, {m_parameters.min_area}, "min area", p});
    m_printer.log_message(
        {i_use, {m_parameters.recurse ? 1 : 0}, "recurse", p});

    // ImageProcessor info
    int nRows = (*image).rows;
    int nCols = (*image).cols;
    int size = nCols * nRows;

    // Preinit
    int imageLeft = size;
    int visited = 0;
    int amount = 0;
    int x, y = 0;
    Stats stats = {visited, amount};
    uchar id = UCHAR_MAX;

    // Preinit
    cv::Mat output = cv::Mat((*image).rows, (*image).cols, CV_8U, double(0));
    Point current = Point(0, 0);
    uchar val = 0;

    m_log.start();

    for (int i = 0; i < nRows * nCols; i++) {
        x = i % nCols;
        y = i / nCols;

        // Skip undesired points
        current = Point(x, y);  //? remove this init?
        val = (*image).at<uchar>(current);
        visited++;
        if (val <= m_parameters.min_distance) continue;
        if (m_objects.at<uchar>(current) != 0) continue;

        m_log.start();
        amount = 0;
        imageLeft = size - y * nCols + x;  // TODO
        if (m_parameters.recurse)
            paint(current, output, id, stats);
        else
            iterate(current, output, imageLeft, id, stats);

        if (amount < m_parameters.min_area) {
            m_printer.log_message({w_smol, {amount, m_parameters.min_area}});
            m_log.drop();
            continue;
        }

        if (mask_mats.size() < m_parameters.max_objects)
            mask_mats.push_back({output, amount});
        else {
            m_printer.log_message({w_limit, {m_parameters.max_objects}});
            m_log.drop();
            continue;
        }

        m_log.stop("seek");
    }

    m_printer.log_message({i_info, {visited}, "visited", p});
    m_printer.log_message(
        {i_info, {(*image).rows * (*image).cols}, "total", p});

    m_log.stop("overall");
    m_log.print();
    m_log.log();
    m_log.flush();

    for (int i = 0; i < mask_mats.size(); i++) {
        imwrite(m_out_path + "mask " + to_string(i) + " .png",
                mask_mats.at(i).mat);
    }

    imwrite(m_out_path + "objects.png", m_objects);
}

void ImageProcessor::pruneMasks() { mask_mats.clear(); }

void ImageProcessor::iterate(Point start, cv::Mat &output, int imageLeft,
                             uchar &id, Stats stats) {
    auto [visited, amount] = stats;

    output = cv::Mat((*image).rows, (*image).cols, CV_8U, double(0));
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

        previous_z = (*image).at<uchar>(pd.coordinates);
        mediumVal = (mediumVal * amount + previous_z) / (amount + 1);
        for (auto dir : pd.directions) {
            point_to_check = pd.coordinates;
            point_to_check += dir;

            visited++;
            // Basic checks
            if (point_to_check.x >= (*image).cols ||
                point_to_check.y >= (*image).rows || point_to_check.x < 0 ||
                point_to_check.y < 0)
                continue;
            if (m_objects.at<uchar>(point_to_check) != 0) continue;
            if ((*image).at<uchar>(point_to_check) <= m_parameters.min_distance)
                continue;
            if (abs((*image).at<uchar>(point_to_check) - previous_z) >
                m_parameters.z_limit)
                continue;

            // Additional checks
            // medium check
            if (abs((*image).at<uchar>(point_to_check) - mediumVal) >
                m_parameters.medium_limit)
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

    output = cv::Mat((*image).rows, (*image).cols, CV_8U, double(0));
    uchar start_z = (*image).at<uchar>(start);
    double mediumVal = start_z;

    walk(output, start_z, mediumVal, start.x, start.y, id, visited, amount);
}

bool ImageProcessor::walk(cv::Mat &output, uchar prev_z, double &mediumVal,
                          int x, int y, uchar &id, int &visited, int &amount) {
    visited++;
    // Basic checks
    if (x >= (*image).cols || y >= (*image).rows || x < 0 || y < 0)
        return false;
    Point current = Point(x, y);
    if (m_objects.at<uchar>(current) != 0) return false;
    if ((*image).at<uchar>(current) <= m_parameters.min_distance) return false;
    if (abs((*image).at<uchar>(current) - prev_z) > m_parameters.z_limit)
        return false;

    // Additional checks
    // medium check
    if (abs((*image).at<uchar>(current) - mediumVal) >
        m_parameters.medium_limit)
        return false;

    m_objects.at<uchar>(current) = id;  // Painting the pixel
    output.at<uchar>(current) = id;     // Painting the pixel
    amount++;
    mediumVal = (mediumVal * amount + prev_z) / (amount + 1);
    // recurse
    for (int i = 0; i < 4; i++) {
        walk(output, (*image).at<uchar>(current), mediumVal,
             current.x + directions[i][0], current.y + directions[i][1], id,
             visited, amount);
    }

    return false;
}