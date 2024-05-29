#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <sl/Camera.hpp>
#include <string>

#include "../../include/sl_utils.hpp"
#include "../impl/object_recognition.cpp"
#include "../impl/utils.cpp"
#include "opencv2/opencv.hpp"

namespace zed {

class CameraManager {
    Printer m_printer;
    Printer::DEBUG_LVL m_prod = Printer::DEBUG_LVL::PRODUCTION;
    Printer::ERROR m_info = Printer::ERROR::INFO;
    Printer::ERROR m_succ = Printer::ERROR::SUCCESS;

    sl::Camera m_zed;
    sl::InitParameters m_initParams;
    sl::RuntimeParameters m_runParams;
    sl::Resolution m_resolution;
    int m_svo_pos;
    HoughLinesPsets m_hough_params;

    bool m_isGrabbed;

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

    CameraManager(Printer printer) : m_printer(printer){};

    // TODO sl::ERROR?
    void openCam(Config config) {
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
        image_mask =
            new sl::Mat(m_resolution, sl::MAT_TYPE::U8_C1, sl::MEM::CPU);
        image_mask_cv = slMat2cvMat(*image_mask);
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
        m_zed.retrieveImage(*image_depth, sl::VIEW::DEPTH);

        if (write) {
            if ((*image_color)
                    .write(("capture_" + std::to_string(m_svo_pos) + ".png")
                               .c_str()) == sl::ERROR_CODE::SUCCESS)
                m_printer.log_message(
                    {m_succ, {}, "color image saving", m_prod});
            if ((*image_depth)
                    .write(
                        ("capture_depth_" + std::to_string(m_svo_pos) + ".png")
                            .c_str()) == sl::ERROR_CODE::SUCCESS)
                m_printer.log_message(
                    {m_succ, {}, "depth image saving", m_prod});
        }
    }

    void calibrate(std::string windowName, std::string saveLocation,
                   int minArea) {
        // TODO provide keyboard controller
        if (!m_zed.isOpened()) throw("Camera is not opened");

        Logger logger;
        ImageProcessor imageProcessor(saveLocation, logger, m_printer);

        cv::Mat white(m_resolution.height, m_resolution.width, CV_8UC4,
                      cv::Scalar(255, 255, 255, 255));

        cv::imshow(windowName, white);
        cv::waitKey(100);
        int biggestMaskIdx = 0;
        while (true) {
            imageProcessor.pruneMasks();

            cv::imshow(windowName, white);
            cv::waitKey(100);

            auto returned_state = grab();
            if (returned_state != sl::ERROR_CODE::SUCCESS)
                throw("Frame is not grabbed");
            m_zed.retrieveImage(*image_gray, sl::VIEW::LEFT_GRAY, sl::MEM::CPU,
                                m_resolution);

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
            imageProcessor.findObjects(10, max - 30, 10, minArea, 4, false);

            int area = 0;
            int maxArea = 0;
            for (int i = 0; i < imageProcessor.mask_mats.size(); i++) {
                area = imageProcessor.mask_mats.at(i).area;
                if (area > maxArea) {
                    maxArea = area;
                    biggestMaskIdx = i;
                }
            }

            std::cout << std::endl << maxArea << std::endl;

            if (maxArea < minArea) {
                continue;
            } else {
                image_mask_cv =
                    imageProcessor.mask_mats.at(biggestMaskIdx).mat.clone();
                try {
                    deduceHomography();
                    break;
                } catch (const std::exception &e) {
                    std::cerr << e.what() << '\n';
                    continue;
                }
            }
        }

        imageProcessor.pruneMasks();
        (*image_mask) = cvMat2slMat(image_mask_cv);
        (*image_mask).write((saveLocation + "ROI_mask.png").c_str());
        m_zed.setRegionOfInterest(*image_mask);
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
        m_initParams.camera_resolution = sl::RESOLUTION::AUTO;
        m_initParams.coordinate_units = sl::UNIT::METER;
        m_initParams.depth_maximum_distance = config.camera_diatance;
        // TODO check for files existance
        if (config.type == Config::SOURCE_TYPE::SVO)
            m_initParams.input.setFromSVOFile((config.file_path).c_str());
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

        // TODO sort ("rotate") points before findHomography

        vector<Point> default_corners = {Point(width, 0), Point(width, height),
                                         Point(0, 0), Point(0, height)};
        if (found_corners.size() == 4) {
            homography = findHomography(found_corners, default_corners);
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
                     j++)  // j starts at i so we don't have duplicated points
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