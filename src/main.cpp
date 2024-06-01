
#include <csignal>
// #include <iostream>

// #include "../include/sl_utils.hpp"
#include "./headers/camera.hpp"
// #include "./headers/converter.hpp"
#include "./headers/settings.hpp"
// #include "./impl/object_recognition.cpp"
#include "./impl/templategen.cpp"
// #include "./impl/utils.cpp"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

using namespace cv;
using namespace std;
// using namespace dm;

// TODO check if an image exists in the location
// TODO frontend
// TODO     show log statistics
// TODO
// TODO backend
// TODO     save test image in a "database"
// TODO
// TODO

// TODO free cam on process kill

// TODO check for cam movement and activate recalibration

// TODO join threads on kill

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

    int moment_in_time = 0;

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
        m_image_processor.getImage(m_settings.config.file_path);
    }

    void zedProc() { zed::CameraManager cam_man(m_printer); }

    void initInteractivity() {
        window_name = "Projection";
        cv::namedWindow(window_name, cv::WindowFlags::WINDOW_NORMAL);
        cv::setWindowProperty(window_name,
                              cv::WindowPropertyFlags::WND_PROP_FULLSCREEN,
                              cv::WindowFlags::WINDOW_FULLSCREEN);
        // Templates templates(image.image);
        m_state.calibrate = true;
    }

    void doTheLoop() {
        // camMan.imageProcessing(false);

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

    std::mutex calibration_mutex;
    std::condition_variable calibration_condition;
    std::atomic<bool> imshow_available{true};

    std::mutex settings_mutex;
    std::condition_variable settings_condition;
    std::atomic<bool> settings_available{true};

    void acquireInformation() {
        zed::CameraManager cam_man(m_printer);

        cam_man.openCam(m_settings.config);

        cv::Mat image = cv::Mat(m_resolution, CV_8UC1, double(0));

        while (m_state.keep_running) {
            // TODO ensure that m_state commands are processed
            if (m_state.load_settings) {
                settings_available = false;
                std::lock_guard<std::mutex> lock(settings_mutex);
                loadSettings(cam_man);
                settings_available = true;
            }
            settings_available.notify_one();
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
            }
            mats_condition.notify_one();

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void showAndControl() {
        cv::Mat image = cv::Mat(m_resolution, CV_8UC1, double(0));

        auto printer = [this](string mode, int value = 0,
                              string value_name = "") {
            m_printer.log_message({Printer::INFO,
                                   {value},
                                   "Using " + mode + " mode; " + value_name,
                                   Printer::DEBUG_LVL::PRODUCTION});
        };

        while (m_state.keep_running) {
            m_state.next = false;
            m_state.idx = 0;

            std::unique_lock<std::mutex> lock_settings(settings_mutex);
            settings_condition.wait(
                lock_settings, [this] { return settings_available.load(); });
            std::unique_lock<std::mutex> lock_imshow(calibration_mutex);
            mats_condition.wait(lock_imshow,
                                [this] { return imshow_available.load(); });

            switch (m_state.mode) {
                case InteractiveState::Mode::NONE: {
                    printer("NONE");
                    image = cv::Mat::zeros(image.size(), CV_8UC4);
                    break;
                }
                case InteractiveState::Mode::WHITE: {
                    uchar brightness = m_state.scales.at(0).second * 25 + 5;
                    printer("WHITE", brightness, "Brightness");
                    image =
                        cv::Mat(image.size(), CV_8UC3,
                                cv::Scalar(brightness, brightness, brightness));
                    break;
                }
                case InteractiveState::Mode::CHESS: {
                    printer("CHESS");
                    cv::Mat white(image.size(), CV_8UC1,
                                  cv::Scalar(255, 255, 255));
                    image = m_templates.chessBoard(0, white);
                    break;
                }
                case InteractiveState::Mode::DEPTH: {
                    printer("DEPTH");
                    break;
                }
                case InteractiveState::Mode::OBJECTS: {
                    printer("OBJECTS");

                    // TODO write a message about lock
                    // TODO create a separate mats for the case of a lock
                    if (!mats_available.load()) break;
                    std::unique_lock<std::mutex> lock_mats(mats_mutex);
                    mats_condition.wait(
                        lock_mats, [this] { return mats_available.load(); });
                    // ! has problems, arrays are off??
                    maskAgregator(image);
                    break;
                }
                case InteractiveState::Mode::TEMPLATES: {
                    printer("TEMPLATES");

                    // TODO write a message about lock
                    // TODO create a separate mats for the case of a lock
                    if (!mats_available.load()) break;
                    std::unique_lock<std::mutex> lock_mats(mats_mutex);
                    mats_condition.wait(
                        lock_mats, [this] { return mats_available.load(); });
                    // ! has problems, arrays are off??
                    applyTemplates(image);
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
            m_printer.log_message({Printer::ERROR::INFO,
                                   {0},
                                   "Loading config",
                                   Printer::DEBUG_LVL::PRODUCTION});
            m_settings.ParseConfig();

            cam_man.updateRunParams(m_settings.config);
            cam_man.updateHough(m_settings.hough_params);
            m_image_processor.setParametersFromSettings(m_settings.config);
            setResolution(static_cast<sl::RESOLUTION>(
                m_settings.config.camera_resolution));
            m_templates.setResolution(m_resolution);
            m_state.load_settings = false;
        } catch (const std::exception &e) {
            std::cerr << e.what() << 'in method \'loadSettings\'\n';
            m_state.load_settings = false;
        }
    }

    void restartCamera(zed::CameraManager &cam_man) {
        try {
            m_printer.log_message({Printer::ERROR::INFO,
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
            cam_man.imageProcessing(false);
            int height = cam_man.image_depth_cv.rows;
            int width = cam_man.image_depth_cv.cols;
            cv::Mat transformed(height, width, CV_8UC1);
            cv::warpPerspective(cam_man.image_depth_cv, transformed,
                                cam_man.homography, cv::Size(width, height));
            image = transformed;

        } catch (const std::exception &e) {
            std::cerr << "Image processing failed; " << e.what()
                      << 'in method \'grabImage\'\n';
        }
    }

    void postProcessing(cv::Mat &image) {
        try {
            m_image_processor.mask_mats.clear();

            if (image.channels() == 4) cvtColor(image, image, COLOR_BGRA2GRAY);
            if (image.channels() == 3) cvtColor(image, image, COLOR_BGR2GRAY);

            m_image_processor.getImage(&image);

            for (auto action : m_settings.erodil) {
                if (action.type == ErosionDilation::Dilation)
                    m_image_processor.dilate(action.distance, action.size);
                else
                    m_image_processor.erode(action.distance, action.size);
            }

            m_image_processor.setParametersFromSettings(m_settings.config);
            m_image_processor.findObjects();

            m_mask_mats = m_image_processor.mask_mats;
        } catch (const std::exception &e) {
            std::cerr << e.what() << 'in method \'postProcessing\'\n';
            m_state.load_settings = false;
        }
    }

    void maskAgregator(cv::Mat &image) {
        try {
            for (auto mask : m_mask_mats) {
                cv::add(image, mask.mat, image);
            }
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
        }
    }

    void applyTemplates(cv::Mat &image) {
        try {
            // TODO color coding for objects via tamplates
            // use settings to define template characteristics

            if (moment_in_time >= 60 * 10) moment_in_time = 0;
            // cleanup; 1 channels
            image = cv::Mat::zeros(image.size(), CV_8UC3);

            cv::Mat result(image.size(), image.type());
            for (auto mask : m_mask_mats) {
                result = m_templates.gradient(moment_in_time, mask.mat, 5);
                cv::add(image, result, image);
            }

            imwrite(m_settings.config.output_location + "templated_image.png",
                    image);

            moment_in_time++;
        } catch (const std::exception &e) {
            std::cerr << e.what() << 'in method \'applyTemplates\'\n';
            m_state.load_settings = false;
        }
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

    auto Result = settings.Init(argc, argv);
    if (Result == Printer::ERROR::ARGS_FAILURE) {
        printer.log_message({Printer::ERROR::ARGS_FAILURE, {}, argv[0]});
        throw runtime_error("Settings failure");
    }
    try {
        settings.Parse();
    } catch (const std::exception &x) {
        printer.log_message({Printer::ERROR::FLAGS_FAILURE, {}, argv[0]});
        printer.log_message(x);
        throw runtime_error("Settings failure");
    }
    int lvl = settings.config.debug_level;
    printer.setDebugLevel(static_cast<Printer::DEBUG_LVL>(lvl));

    Logger logger("log", 0, 0, settings.config.save_logs,
                  settings.config.measure_time, settings.config.debug_level);
    ImageProcessor image_processor(settings.config.output_location, logger,
                                   printer);
    // Templates templates(settings.config.resolution);
    Templates templates({1920, 1080});

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
