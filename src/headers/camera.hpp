#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <array>
#include <deque>
#include <optional>
#include <sl/Camera.hpp>
#include <string>

#include "../../include/sl_utils.hpp"
#include "../headers/settings.hpp"
#include "../impl/object_recognition.cpp"
#include "../impl/utils.cpp"

namespace zed {

class CameraManager {
    Printer m_printer;
    Printer::DEBUG_LVL m_prod = Printer::DEBUG_LVL::PRODUCTION;
    Printer::ERROR m_info = Printer::ERROR::INFO;
    Printer::ERROR m_succ = Printer::ERROR::SUCCESS;
    Printer::ERROR m_fail = Printer::ERROR::FAILURE;

    sl::Camera m_zed;
    sl::InitParameters m_initParams;
    sl::RuntimeParameters m_runParams;
    sl::Resolution m_resolution;
    int m_svo_pos;
    HoughLinesPsets m_hough_params;

    bool m_isGrabbed;
    int m_repeat_times;

    std::deque<cv::Mat> m_image_que;

   public:
    sl::Mat *image_color;
    cv::Mat image_color_cv;
    sl::Mat *image_gray;
    cv::Mat image_gray_cv;
    sl::Mat *image_depth;
    cv::Mat image_depth_cv;
    sl::Mat *image_mask;
    cv::Mat image_mask_cv;

    cv::Mat homography;

    CameraManager() = default;
    CameraManager(Printer printer) : m_printer(printer){};

    // TODO sl::ERROR?
    void openCam(Config config) {
        m_repeat_times = config.repeat_times;
        setInitParams(config);
        auto returned_state = m_zed.open(m_initParams);
        if (returned_state != sl::ERROR_CODE::SUCCESS) {
            throw(returned_state);
        }
        setRunParams(config);
        m_isGrabbed = false;

        image_color =
            new sl::Mat(m_resolution, sl::MAT_TYPE::U8_C4, sl::MEM::CPU);
        image_color_cv = slMat2cvMat(*image_color);
        image_gray =
            new sl::Mat(m_resolution, sl::MAT_TYPE::U8_C1, sl::MEM::CPU);
        image_gray_cv = slMat2cvMat(*image_gray);
        image_depth =
            new sl::Mat(m_resolution, sl::MAT_TYPE::U8_C4, sl::MEM::CPU);
        image_depth_cv = slMat2cvMat(*image_depth);

        image_mask_cv =
            cv::Mat::ones(m_resolution.height, m_resolution.height, CV_8U);
    }

    void restartCamera(Config config) {
        m_zed.close();
        openCam(config);
        // TODO destroy some things?
    }

    void updateRunParams(Config config) { setRunParams(config); }

    void updateHough(HoughLinesPsets hough_params) {
        m_hough_params = hough_params;
    }

    sl::ERROR_CODE grab() {
        auto returned_state = m_zed.grab(m_runParams);
        if (returned_state == sl::ERROR_CODE::SUCCESS)
            m_isGrabbed = true;
        else
            return returned_state;

        return sl::ERROR_CODE::SUCCESS;
    }

    void imageProcessing(bool write = false) {
        if (!m_zed.isOpened()) throw("Camera is not opened");
        if (!m_isGrabbed) throw("Frame is not grabbed");

        m_zed.retrieveImage(*image_color, sl::VIEW::LEFT, sl::MEM::CPU,
                            m_resolution);
        m_zed.retrieveImage(*image_gray, sl::VIEW::LEFT_GRAY, sl::MEM::CPU,
                            m_resolution);
        m_zed.retrieveImage(*image_depth, sl::VIEW::DEPTH, sl::MEM::CPU,
                            m_resolution);

        m_image_que.push_back(image_depth_cv);
        if (m_image_que.size() > m_repeat_times) {
            m_image_que.pop_front();
        }
        if (m_image_que.size() == m_repeat_times) {
            auto agregator = m_image_que.front();

            for (int i = 1; i < m_image_que.size(); i++) {
                auto image = m_image_que.at(i);
                cv::bitwise_and(image, agregator, agregator);
            }

            image_depth_cv = agregator;
        }

        // sl::Mat measure_depth;
        // auto returned_state = m_zed.retrieveMeasure(
        //     measure_depth, sl::MEASURE::DEPTH, sl::MEM::CPU,
        //     m_resolution);

        // if (returned_state == sl::ERROR_CODE::SUCCESS) {
        //     measure_depth.write("./Result/DEPTH_MEASURE.png",
        //     sl::MEM::CPU);
        // }

        if (write) {
            if ((*image_color)
                    .write("./Result/Color_image.png", sl::MEM::CPU) ==
                sl::ERROR_CODE::SUCCESS)
                m_printer.log_message(
                    {m_succ, {0}, "Color image save", m_prod});
            else
                m_printer.log_message(
                    {m_fail, {0}, "Color image save", m_prod});
            if ((*image_depth)
                    .write("./Result/Depth_image.png", sl::MEM::CPU) ==
                sl::ERROR_CODE::SUCCESS)
                m_printer.log_message(
                    {m_succ, {0}, "Depth image save", m_prod});
            else
                m_printer.log_message(
                    {m_fail, {0}, "Depth image save", m_prod});
        }
    }

    void calibrate(std::string window_name, InteractiveState &state,
                   std::string save_location, int min_area) {
        // TODO provide keyboard controller
        if (!m_zed.isOpened()) throw("Camera is not opened");

        Logger logger;
        ImageProcessor imageProcessor(save_location, logger, m_printer);

        cv::Mat white(m_resolution.height, m_resolution.width, CV_8UC4,
                      cv::Scalar(255, 255, 255, 255));

        cv::imshow(window_name, white);
        state.key = cv::waitKey(100);
        int biggestMaskIdx = 0;
        while (state.calibrate) {
            imageProcessor.pruneMasks();
            cout << "Calibrating!" << endl;
            cv::imshow(window_name, white);
            state.key = cv::waitKey(100);
            state.action();

            auto returned_state = grab();
            if (returned_state != sl::ERROR_CODE::SUCCESS)
                throw("Frame is not grabbed");
            m_zed.retrieveImage(*image_gray, sl::VIEW::LEFT_GRAY, sl::MEM::CPU,
                                m_resolution);

            double alpha = 3.0;
            int beta = 0;

            cv::Mat normalized_image =
                cv::Mat::zeros(image_gray_cv.size(), image_gray_cv.type());

            auto mean = cv::mean(image_gray_cv)[0];
            std::cout << mean << std::endl;
            // cv::threshold(image_gray_cv, normalized_image, mean + 20, 255,
            //               ThresholdTypes::THRESH_BINARY);

            imwrite("./Result/input_init.png", image_gray_cv);
            imwrite("./Result/input_norm.png", normalized_image);

            int value = 0;
            int max = 0;
            int x = 0;
            int y = 0;
            int nRows = m_resolution.height;
            int nCols = m_resolution.width;
            for (int i = 0; i < nRows * nCols; i++) {
                x = i % nCols;
                y = i / nCols;

                value = (int)image_gray_cv.at<uchar>(y, x);
                if (value > max) max = value;
            }

            imageProcessor.getImage(&image_gray_cv);
            Config cfg{.z_limit = 10,
                       .min_distance = uchar(max - uchar((max - mean) / 2)),
                       .medium_limit = 10,
                       .min_area = min_area,
                       .max_objects = 4,
                       .recurse = false};
            imageProcessor.setParametersFromSettings(cfg);
            imageProcessor.findObjects();

            int area = 0;
            int max_area = 0;
            for (int i = 0; i < imageProcessor.mask_mats.size(); i++) {
                area = imageProcessor.mask_mats.at(i).area;
                if (area > max_area) {
                    max_area = area;
                    biggestMaskIdx = i;
                }
            }

            std::cout << std::endl << max_area << std::endl;

            if (max_area < min_area) {
                continue;
            } else {
                image_mask_cv =
                    imageProcessor.mask_mats.at(biggestMaskIdx).mat.clone();
                try {
                    deduceHomography();
                    state.calibrate = false;
                    break;
                } catch (const std::exception &e) {
                    std::cerr << e.what() << '\n';
                    continue;
                }
            }
        }

        if (state.calibrate) {
            imageProcessor.pruneMasks();
            (*image_mask) = cvMat2slMat(image_mask_cv);
            (*image_mask).write((save_location + "ROI_mask.png").c_str());
            m_zed.setRegionOfInterest(
                *image_mask,
                {sl::MODULE::POSITIONAL_TRACKING, sl::MODULE::SPATIAL_MAPPING});
        }
    }

    ~CameraManager() {
        // TODO cleanup sl::Mat pointers
        m_zed.close();
    }

   private:
    // TODO error??
    void setInitParams(Config config) {
        m_initParams.depth_mode =
            static_cast<sl::DEPTH_MODE>(config.depth_mode);
        m_initParams.coordinate_system =
            sl::COORDINATE_SYSTEM::RIGHT_HANDED_Y_UP;
        m_initParams.sdk_verbose = 1;  // TODO cli?
        m_initParams.camera_resolution =
            static_cast<sl::RESOLUTION>(config.camera_resolution);
        m_initParams.coordinate_units = sl::UNIT::METER;
        m_initParams.depth_maximum_distance = config.camera_diatance;
        // TODO check for files existance
        if (config.type == Config::SOURCE_TYPE::SVO)
            m_initParams.input.setFromSVOFile((config.file_path).c_str());

        // Should reduce computation
        sl::PositionalTrackingParameters PosParams;
        PosParams.set_as_static = true;
        m_zed.enablePositionalTracking(PosParams);
    }

    void initSVO() {
        int nb_frames = m_zed.getSVONumberOfFrames();
        m_svo_pos = (int)nb_frames / 2;  // TODO cli??

        m_printer.log_message(
            {Printer::INFO, {nb_frames}, "SVO contains (frames)", m_prod});
        m_printer.log_message(
            {Printer::INFO, {m_svo_pos}, "SVO position (frame)", m_prod});

        m_zed.setSVOPosition(m_svo_pos);
    }

    void setRunParams(Config config) {
        m_runParams.confidence_threshold = config.threshold;
        m_runParams.texture_confidence_threshold = config.texture_threshold;
        m_runParams.enable_fill_mode = config.fill_mode;

        // TODO scale resolution
        m_resolution =
            m_zed.getCameraInformation().camera_configuration.resolution;

        if (config.type == Config::SOURCE_TYPE::SVO) initSVO();
    }

    void deduceHomography() {
        cv::Mat border;
        cv::Canny(image_mask_cv, border, 30, 100);
        int width = image_mask_cv.cols;
        int height = image_mask_cv.rows;

        cv::Mat out(image_mask_cv);
        vector<Point> found_corners;
        auto returned_status = findCorners(out, found_corners);

        if (returned_status != sl::ERROR_CODE::SUCCESS) {
            imwrite("./Result/input.png", out);
            throw std::runtime_error("Failed to find corners");
        }

        vector<Point> default_corners = {Point(width, 0), Point(width, height),
                                         Point(0, 0), Point(0, height)};

        int dist_threshold = 10;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; i < 4; i++) {
                if (i == j) continue;

                auto p1 = found_corners.at(i);
                auto p2 = found_corners.at(j);
                if ((abs(p1.x - p2.x) < dist_threshold) &&

                    (abs(p1.y - p2.y) < dist_threshold))
                    throw runtime_error("Points too close");
            }
        }

        auto distance2 = [](cv::Point A, cv::Point B) -> double {
            return ((A.x - B.x) * (A.x - B.x) + (A.y - B.y) * (A.y - B.y));
        };

        vector<Point> sorted_found_corners(4, Point(0, 0));
        array<optional<int>, 4> indices;

        for (int d_idx = 0; d_idx < default_corners.size(); d_idx++) {
            auto def_point = default_corners.at(d_idx);
            double min_dist = INT32_MAX;
            for (int f_idx = 0; f_idx < found_corners.size(); f_idx++) {
                auto found_point = found_corners.at(f_idx);
                double distance = distance2(def_point, found_point);

                if (distance == min_dist)
                    throw runtime_error("Bad point location");
                if (distance < min_dist) {
                    min_dist = distance;
                    indices.at(d_idx) = f_idx;
                }
            }
        }

        std::cout << "Indices: ";
        for (auto index : indices) {
            if (index) std::cout << *index << "; ";
        }
        std::cout << "\n";

        // TODO check if points are consecutive
        for (int i = 0; i < indices.size(); i++) {
            if (indices.at(i)) {
                sorted_found_corners.at(i) = found_corners.at(*indices.at(i));
            }
        }

        std::cout << "Points before: ";
        for (auto point : found_corners) {
            std::cout << "x: " << point.x << "y: " << point.y << "; ";
        }
        std::cout << "\n";
        std::cout << "Points after: ";
        for (auto point : sorted_found_corners) {
            std::cout << "x: " << point.x << "y: " << point.y << "; ";
        }
        std::cout << "\n";

        if (found_corners.size() == 4) {
            homography = findHomography(sorted_found_corners, default_corners);
        }

        imwrite("./Result/input.png", out);
    }

    Vec3f calcParams(Point2f p1,
                     Point2f p2)  // line's equation Params computation
    {
        float a, b, c;
        if (p2.y - p1.y == 0) {
            a = 0.0f;
            b = -1.0f;
        } else if (p2.x - p1.x == 0) {
            a = -1.0f;
            b = 0.0f;
        } else {
            a = (p2.y - p1.y) / (p2.x - p1.x);
            b = -1.0f;
        }

        c = (-a * p1.x) - b * p1.y;
        return (Vec3f(a, b, c));
    }

    Point findIntersection(Vec3f params1, Vec3f params2) {
        float x = -1, y = -1;
        float det = params1[0] * params2[1] - params2[0] * params1[1];
        if (det < 0.5f && det > -0.5f)  // lines are approximately parallel
        {
            return (Point(-1, -1));
        } else {
            x = (params2[1] * -params1[2] - params1[1] * -params2[2]) / det;
            y = (params1[0] * -params2[2] - params2[0] * -params1[2]) / det;
        }
        return (Point(x, y));
    }

    sl::ERROR_CODE findCorners(cv::Mat &out, vector<Point> &corners) {
        // Guaranteed to have one shape at this point
        int width = image_mask_cv.cols;
        int height = image_mask_cv.rows;

        Mat image_hull(height, width, CV_8UC1);
        image_hull = Scalar(0);

        vector<vector<Point>> contours(1);
        vector<vector<Point>> hull(1);
        findContours(image_mask_cv, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);
        convexHull(Mat(contours.at(0)), hull[0], false);

        vector<Vec4i> lines;
        drawContours(image_hull, hull, 0, Scalar(255));
        imwrite("./Result/hull.png", image_hull);
        cv::HoughLinesP(
            image_hull, lines, m_hough_params.rho,
            CV_PI / m_hough_params.theta_denom, m_hough_params.threshold,
            m_hough_params.min_line_length, m_hough_params.max_line_gap);
        cout << "lines size:" << lines.size() << endl;

        cv::Mat img_with_lines = out;
        for (size_t i = 0; i < lines.size(); i++) {
            Vec4i l = lines[i];
            line(img_with_lines, Point(l[0], l[1]), Point(l[2], l[3]),
                 Scalar(122, 122, 122), 3, LINE_AA);
        }
        imwrite("./Result/lines.png", img_with_lines);

        if (lines.size() == 4)  // we found the 4 sides
        {
            vector<Vec3f> params(4);
            for (int l = 0; l < 4; l++) {
                params.push_back(calcParams(Point(lines[l][0], lines[l][1]),
                                            Point(lines[l][2], lines[l][3])));
            }

            // vector<Point> corners;
            for (int i = 0; i < params.size(); i++) {
                for (int j = i; j < params.size();
                     j++)  // j starts at i so we don't have duplicated
                           // points
                {
                    Point intersec = findIntersection(params[i], params[j]);
                    if ((intersec.x > 0) && (intersec.y > 0) &&
                        (intersec.x < width) && (intersec.y < height)) {
                        cout << "corner: " << intersec << endl;
                        corners.push_back(intersec);
                    }
                }
            }
            for (int i = 0; i < corners.size(); i++) {
                circle(out, corners[i], 3, Scalar(122, 122, 122));
            }

            if (corners.size() == 4)  // we have the 4 final corners
            {
                return sl::ERROR_CODE::SUCCESS;
            }
        }

        return sl::ERROR_CODE::FAILURE;
    }
};

}  // namespace zed

#endif  // CAMERA_HPP