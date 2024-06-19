
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <csignal>
#include <mutex>
#include <thread>

#include "./headers/camera.hpp"
#include "./headers/settings.hpp"
#include "./impl/templategen.cpp"

using namespace cv;
using namespace std;

void signalHandler(int signalNumber);

class Loop {
    Settings m_settings;
    Printer m_printer;
    Logger m_logger;
    ImageProcessor m_image_processor;
    Templates m_templates;

    InteractiveState m_state;

    vector<ImageProcessor::MatWithInfo> m_mask_mats;
    cv::Size m_resolution = {1280, 720};

   public:
    string window_name;

   public:
    Loop(Settings sets, Printer print, Logger log, ImageProcessor img_proc,
         Templates templates)
        : m_settings(sets),
          m_printer(print),
          m_logger(log),
          m_image_processor(img_proc),
          m_templates(templates) {
        setResolution(
            static_cast<sl::RESOLUTION>(m_settings.config.camera_resolution));
    }

    void Process() {
        auto type = m_settings.config.type;
        switch (type) {
            case Config::SOURCE_TYPE::IMAGE:
                imageProc();
                break;
            case Config::SOURCE_TYPE::SVO:
                zedProc();
                break;
            case Config::SOURCE_TYPE::STREAM: {
                initInteractivity();
                zedProc();
                doTheLoop();
                // cleanup();
            } break;
            default:
                break;
        }
    }

    ~Loop() {}

   private:
    void imageProc() {
        m_image_processor.setImage(m_settings.config.file_path);
    }

    void zedProc() { zed::CameraManager cam_man(m_printer); }

    void initInteractivity() {
        window_name = "Projection";
        cv::namedWindow(window_name, cv::WindowFlags::WINDOW_NORMAL);
        cv::setWindowProperty(window_name,
                              cv::WindowPropertyFlags::WND_PROP_FULLSCREEN,
                              cv::WindowFlags::WINDOW_FULLSCREEN);
        m_state.calibrate = true;
    }

    void doTheLoop() {
        m_state.printHelp();

        std::thread readerThread(&Loop::acquireInformation, this);
        std::thread displayThread(&Loop::showAndControl, this);

        // Wait for the threads to finish
        readerThread.join();
        displayThread.join();
    }

    std::mutex mats_mutex;
    std::condition_variable mats_condition;
    std::atomic<bool> mats_available{false};
    std::atomic<bool> mats_changed{false};

    std::mutex calibration_mutex;
    std::condition_variable calibration_condition;
    std::atomic<bool> imshow_available{true};

    std::mutex settings_mutex;
    std::condition_variable settings_condition;
    std::atomic<bool> settings_available{true};

    void acquireInformation() {
        zed::CameraManager cam_man(m_printer);

        cam_man.openCam(m_settings.config);

        cv::Mat image(m_resolution, CV_8UC1, double(0));

        while (m_state.keep_running) {
            // TODO ensure that m_state commands are processed
            if (m_state.load_settings) {
                settings_available = false;
                std::lock_guard<std::mutex> lock(settings_mutex);
                loadSettings(cam_man);
                settings_available = true;
            }
            settings_condition.notify_one();
            if (m_state.restart_cam) restartCamera(cam_man);
            if (m_state.calibrate) {
                imshow_available = false;
                std::lock_guard<std::mutex> lock(calibration_mutex);
                calibrate(cam_man);
                imshow_available = true;
            }
            calibration_condition.notify_one();

            auto returned_state = cam_man.grab();
            if (m_state.grab && returned_state == sl::ERROR_CODE::SUCCESS)
                grabImage(cam_man, image);

            if (m_state.process) {
                mats_available = false;
                std::lock_guard<std::mutex> lock(mats_mutex);
                postProcessing(image);
                mats_available = true;
                mats_changed = true;
            }
            mats_condition.notify_one();

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void showAndControl() {
        cv::Mat image(m_resolution, CV_8UC3, double(0));

        auto print_mode = [this](string mode, int value = 0,
                                 string value_name = "") {
            m_printer.logMessage({Printer::INFO,
                                  {value},
                                  "Using " + mode + " mode; " + value_name,
                                  Printer::DEBUG_LVL::PRODUCTION});
        };

        vector<ImageProcessor::MatWithInfo> mask_mats = m_mask_mats;
        vector<ImageProcessor::MatWithInfo> old_mask_mats;

        cv::Mat image_none = cv::Mat::zeros(image.size(), CV_8UC4);

        while (m_state.keep_running) {
            m_state.next = false;
            m_state.idx = 0;

            if (mats_changed && mats_available) {
                old_mask_mats = mask_mats;
                mask_mats = m_mask_mats;

                m_printer.logMessage({Printer::INFO,
                                      {(int)mask_mats.size()},
                                      "Changed masks, size",
                                      Printer::DEBUG_LVL::PRODUCTION});
                mats_changed = false;
            }

            std::unique_lock<std::mutex> lock_settings(settings_mutex);
            settings_condition.wait(
                lock_settings, [this] { return settings_available.load(); });
            std::unique_lock<std::mutex> lock_imshow(calibration_mutex);
            calibration_condition.wait(
                lock_imshow, [this] { return imshow_available.load(); });

            switch (m_state.mode) {
                case InteractiveState::Mode::NONE: {
                    print_mode("NONE");
                    image = image_none;
                    break;
                }
                case InteractiveState::Mode::WHITE: {
                    uchar brightness = m_state.scales.at(0).second * 25 + 5;
                    print_mode("WHITE", brightness, "Brightness");
                    image =
                        cv::Mat(image.size(), CV_8UC3,
                                cv::Scalar(brightness, brightness, brightness));
                    break;
                }
                case InteractiveState::Mode::CHESS: {
                    print_mode("CHESS");
                    cv::Mat white(image.size(), CV_8UC1,
                                  cv::Scalar(255, 255, 255));
                    image = m_templates.chessBoard(0, white);
                    break;
                }
                case InteractiveState::Mode::DEPTH: {
                    print_mode("DEPTH");

                    image = *m_image_processor.image;
                    break;
                }
                case InteractiveState::Mode::OBJECTS: {
                    print_mode("OBJECTS");

                    // ! has problems, arrays are off??
                    maskAgregator(image, mask_mats);
                    break;
                }
                case InteractiveState::Mode::TEMPLATES: {
                    print_mode("TEMPLATES");

                    applyTemplates(image, mask_mats, old_mask_mats);
                    break;
                }
                default:
                    break;
            }

            imshow(window_name, image);
            m_state.key = cv::waitKey(10);
            m_state.action();
        }
    }

    void loadSettings(zed::CameraManager &cam_man) {
        try {
            m_printer.logMessage({Printer::ERROR::INFO,
                                  {0},
                                  "Loading config",
                                  Printer::DEBUG_LVL::PRODUCTION});
            m_settings.ParseConfig();

            cam_man.updateRunParams(m_settings.config);
            cam_man.updateHough(m_settings.hough_params);
            m_image_processor.setParameters(m_settings.config);
            setResolution(static_cast<sl::RESOLUTION>(
                m_settings.config.camera_resolution));
            m_state.load_settings = false;
        } catch (const std::exception &e) {
            std::cerr << e.what() << 'in method \'loadSettings\'\n';
            m_state.load_settings = false;
        }
    }

    void restartCamera(zed::CameraManager &cam_man) {
        try {
            m_printer.logMessage({Printer::ERROR::INFO,
                                  {0},
                                  "Restarting camera",
                                  Printer::DEBUG_LVL::PRODUCTION});
            m_settings.ParseConfig();

            cam_man.restartCamera(m_settings.config);
            cam_man.updateHough(m_settings.hough_params);
            m_state.restart_cam = false;
        } catch (const std::exception &e) {
            std::cerr << e.what() << 'in method \'restartCamera\'\n';
            m_state.restart_cam = false;
        }
    }

    void calibrate(zed::CameraManager &cam_man) {
        try {
            cam_man.calibrate(window_name, m_state,
                              m_settings.config.output_location, 15000);
            m_state.calibrate = false;
        } catch (const std::exception &e) {
            std::cerr << "Calibration failed; " << e.what()
                      << 'in method \'calibrate\'\n';
            m_state.calibrate = false;
        }
    }

    void grabImage(zed::CameraManager &cam_man, cv::Mat &image) {
        try {
            m_logger.start();
            m_logger.start();
            cam_man.imageProcessing(false);
            m_logger.stop("imageProcessing");
            cv::Mat transformed(cam_man.image_depth_cv.size(), CV_8UC1);
            cv::warpPerspective(cam_man.image_depth_cv, transformed,
                                cam_man.homography, transformed.size());
            image = transformed;
            m_logger.stop("whole grabImage");
            m_logger.print();
            m_logger.flush();

        } catch (const std::exception &e) {
            m_logger.print();
            m_logger.flush();
            std::cerr << "Image processing failed; " << e.what()
                      << 'in method \'grabImage\'\n';
        }
    }

    void postProcessing(cv::Mat &image) {
        try {
            m_image_processor.mask_mats.clear();

            if (image.channels() == 4) cvtColor(image, image, COLOR_BGRA2GRAY);
            if (image.channels() == 3) cvtColor(image, image, COLOR_BGR2GRAY);

            m_image_processor.setImage(&image);

            for (auto action : m_settings.erodil) {
                if (action.type == ErosionDilation::Dilation)
                    m_image_processor.dilate(action.distance, action.size);
                else
                    m_image_processor.erode(action.distance, action.size);
            }

            m_image_processor.setParameters(m_settings.config);
            m_image_processor.findObjects();

            m_mask_mats = m_image_processor.mask_mats;
        } catch (const std::exception &e) {
            std::cerr << e.what() << 'in method \'postProcessing\'\n';
            m_state.load_settings = false;
        }
    }

    void maskAgregator(cv::Mat &image,
                       vector<ImageProcessor::MatWithInfo> &mask_mats) {
        try {
            for (auto mask : mask_mats) {
                image = cv::Mat::zeros(image.size(), CV_8UC3);
                cv::add(image, mask.mat, image);
            }
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
        }
    }

    void applyTemplates(cv::Mat &image,
                        vector<ImageProcessor::MatWithInfo> &mask_mats,
                        vector<ImageProcessor::MatWithInfo> &old_mask_mats) {
        try {
            image = cv::Mat::zeros(image.size(), CV_8UC3);
            vector<cv::Mat> mats;
            for (int i = 0; i < mask_mats.size(); i++) {
                // if (i == 0) continue;
                auto mask = mask_mats.at(i);
                mats.push_back(mask.mat);
            }

            m_templates.applyTemplates(m_settings.templates, mats, image);

            imwrite(m_settings.config.output_location + "templated_image.png",
                    image);

        } catch (const std::exception &e) {
            std::cerr << e.what() << 'in method \'applyTemplates\'\n';
            m_state.load_settings = false;
        }
        // try {
        //     // TODO color coding for objects via tamplates
        //     // use settings to define template characteristics
        //     // cleanup; 1 channels
        //     image = cv::Mat::zeros(image.size(), CV_8UC3);
        //     vector<ImageProcessor::MatWithInfo> mask_mats_clean;
        //     vector<ImageProcessor::MatWithInfo> old_mats_clean;

        //     int bigger_size = mask_mats.size() > old_mask_mats.size()
        //                           ? mask_mats.size()
        //                           : old_mask_mats.size();

        //     vector<optional<ImageProcessor::MatWithInfo>> new_mask_mats(
        //         bigger_size);
        //     vector<optional<cv::Mat>> mats;

        //     for (int i = 0;
        //          i < mask_mats.size() && i < m_settings.config.max_objects;
        //          i++) {
        //         if (i == 0) continue;
        //         auto mask = mask_mats.at(i);
        //         mask_mats_clean.push_back(mask);

        //         // auto mask = mask_mats.at(i);
        //         // mats.push_back(mask.mat);
        //     }

        //     if (!old_mask_mats.empty()) {
        //         auto distance2 = [](cv::Point A, cv::Point B) -> double {
        //             return ((A.x - B.x) * (A.x - B.x) +
        //                     (A.y - B.y) * (A.y - B.y));
        //         };

        //         for (int i = 0; i < old_mask_mats.size() &&
        //                         i < m_settings.config.max_objects;
        //              i++) {
        //             if (i == 0) continue;
        //             auto mask = old_mask_mats.at(i);
        //             old_mats_clean.push_back(mask);
        //         }

        //         vector<optional<int>> indices(1000, nullopt);
        //         vector<bool> are_accounted(mask_mats_clean.size(), false);

        //         for (int d_idx = 0; d_idx < old_mats_clean.size(); d_idx++) {
        //             auto old_mat = old_mats_clean.at(d_idx);
        //             double min_dist = INT32_MAX;
        //             for (int f_idx = 0; f_idx < mask_mats_clean.size();
        //                  f_idx++) {
        //                 auto new_mat = mask_mats_clean.at(f_idx);
        //                 double distance =
        //                     distance2(old_mat.center, new_mat.center);

        //                 if (distance == min_dist)
        //                     throw runtime_error("Bad point location");
        //                 if (distance < min_dist) {
        //                     min_dist = distance;
        //                     indices.at(d_idx) = f_idx;
        //                     are_accounted.at(f_idx) = true;
        //                 }
        //             }
        //         }

        //         for (int i = 0; i < indices.size(); i++) {
        //             if (indices.at(i)) {
        //                 new_mask_mats.at(i) =
        //                     mask_mats_clean.at(*indices.at(i));
        //             }
        //         }

        //         for (int i = 0; i < are_accounted.size(); i++) {
        //             if (are_accounted.at(i)) continue;
        //             new_mask_mats.push_back(mask_mats_clean.at(i));
        //         }

        //         for (auto mask : new_mask_mats) {
        //             if (mask)
        //                 mats.push_back((*mask).mat);
        //             else
        //                 mats.push_back(nullopt);
        //         }
        //     } else {
        //         for (auto mask : mask_mats_clean) {
        //             mats.push_back(mask.mat);
        //         }
        //     }

        //     m_templates.applyTemplates(m_settings.templates, mats, image);

        //     imwrite(m_settings.config.output_location +
        //     "templated_image.png",
        //             image);
        // } catch (const std::exception &e) {
        //     std::cerr << e.what() << 'in method \'applyTemplates\'\n';
        //     m_state.load_settings = false;
        // }
    }

    void setResolution(sl::RESOLUTION resolution) {
        switch (resolution) {
            case sl::RESOLUTION::HD720:
                m_resolution = {1280, 720};
                break;
            case sl::RESOLUTION::HD1080:
                m_resolution = {1920, 1080};
                break;

            default:
                throw runtime_error("Unsupported resolution");
                break;
        }
    }
};

int main(int argc, char **argv) {
    // TODO add more signals, work on handling
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);

    Settings settings;
    Printer printer;

    try {
        settings.Init(argc, argv);
        settings.Parse();
    } catch (const std::exception &x) {
        printer.logMessage({Printer::ERROR::FLAGS_FAILURE, {}, argv[0]});
        printer.logMessage(x);
        throw runtime_error("Settings failure");
    }
    int lvl = settings.config.debug_level;
    printer.setDebugLevel(static_cast<Printer::DEBUG_LVL>(lvl));

    Logger logger("log", 0, 0, settings.config.save_logs,
                  settings.config.measure_time, settings.config.debug_level);
    ImageProcessor image_processor(settings.config.output_location, logger,
                                   printer);
    // Templates templates(settings.config.resolution);
    Templates templates;

    Loop loop(settings, printer, logger, image_processor, templates);
    loop.Process();
    return EXIT_SUCCESS;
}

// TODO graceful stop: destroy the class, join its threads
void signalHandler(int signalNumber) {
    if (signalNumber == SIGTERM || signalNumber == SIGHUP) {
        std::cout << "Recieved interupt signal: " << signalNumber
                  << "\nExiting..." << std::endl;
        exit(signalNumber);
    }
}
