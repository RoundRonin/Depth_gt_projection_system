#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <sl/Camera.hpp>
#include <string>

#include "../../include/sl_utils.hpp"
#include "../impl/object_recognition.cpp"
#include "../impl/utils.cpp"
#include "opencv2/opencv.hpp"

namespace zed {

// TODO refactor
struct Parameters {
    sl::DEPTH_MODE depth_mode = sl::DEPTH_MODE::ULTRA;
    int max_distance = 20;
    int threshold = 10;
    int texture_threshold = 100;
    bool fill_mode = false;
    bool isSVO = false;
    sl::String file_path;
    int resolutionQ = 1;  // TODO handle resolution setting;
};

class CameraManager {
    Printer m_printer;
    Printer::DEBUG_LVL m_prod = Printer::DEBUG_LVL::PRODUCTION;
    Printer::ERROR m_info = Printer::ERROR::INFO;
    Printer::ERROR m_succ = Printer::ERROR::SUCCESS;

    sl::Camera m_zed;
    sl::InitParameters m_initParams;
    sl::RuntimeParameters m_runParams;
    sl::Resolution m_resolution;
    Parameters m_params;
    int m_svo_pos;

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

    CameraManager(Parameters params, Printer printer)
        : m_params(params), m_printer(printer){};

    // TODO sl::ERROR?
    void openCam() {
        setInitParams(m_params);
        auto returned_state = m_zed.open(m_initParams);
        if (returned_state != sl::ERROR_CODE::SUCCESS) {
            throw(returned_state);
        }
        setRunParams(m_params);
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

            if (maxArea < minArea)
                continue;
            else
                break;
        }

        image_mask_cv = imageProcessor.mask_mats.at(biggestMaskIdx).mat.clone();
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
    void setInitParams(Parameters params) {
        m_initParams.depth_mode = params.depth_mode;
        m_initParams.coordinate_system =
            sl::COORDINATE_SYSTEM::RIGHT_HANDED_Y_UP;
        m_initParams.sdk_verbose = 1;  // TODO cli?
        m_initParams.camera_resolution = sl::RESOLUTION::AUTO;
        m_initParams.coordinate_units = sl::UNIT::METER;
        m_initParams.depth_maximum_distance = params.max_distance;
        // TODO check for files existance
        if (params.isSVO) m_initParams.input.setFromSVOFile(m_params.file_path);
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

    void setRunParams(Parameters params) {
        m_runParams.confidence_threshold = params.threshold;
        m_runParams.texture_confidence_threshold = params.texture_threshold;
        m_runParams.enable_fill_mode = params.fill_mode;

        // TODO scale resolution
        m_resolution =
            m_zed.getCameraInformation().camera_configuration.resolution;

        if (params.isSVO) initSVO();
    }
};

}  // namespace zed

#endif  // CAMERA_HPP