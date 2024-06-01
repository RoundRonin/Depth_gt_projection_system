
#include <csignal>
// #include <iostream>

// #include "../include/sl_utils.hpp"
#include "./headers/camera.hpp"
// #include "./headers/converter.hpp"
#include "./headers/settings.hpp"
// #include "./impl/object_recognition.cpp"
#include "./impl/templategen.cpp"
// #include "./impl/utils.cpp"

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

void signalHandler(int signalNumber);

class Loop {
    Settings m_settings;
    Printer m_printer;
    Logger m_logger;
    ImageProcessor m_image_processor;

    InteractiveState m_state;

   public:
    string window_name;

   public:
    Loop(Settings sets, Printer print, Logger log, ImageProcessor img_proc)
        : m_settings(sets),
          m_printer(print),
          m_logger(log),
          m_image_processor(img_proc) {}

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
        zed::CameraManager cam_man(m_printer);

        cam_man.openCam(m_settings.config);
        // camMan.imageProcessing(false);

        m_state.printHelp();
        while (m_state.keep_running) {
            m_state.next = false;
            m_state.idx = 0;

            if (m_state.load_settings) loadSettings(cam_man);
            if (m_state.restart_cam) restartCamera(cam_man);
            if (m_state.calibrate) {
                try {
                    cam_man.calibrate(window_name, m_state,
                                      m_settings.config.output_location, 15000);
                    m_state.calibrate = false;
                } catch (const std::exception &e) {
                    std::cerr << "Calibration failed; " << e.what() << '\n';
                    m_state.calibrate = false;
                }
            }

            cv::Mat image;
            auto returned_state = cam_man.grab();
            if (m_state.grab && returned_state == sl::ERROR_CODE::SUCCESS) {
                try {
                    cam_man.imageProcessing(false);
                    int height = cam_man.image_depth_cv.rows;
                    int width = cam_man.image_depth_cv.cols;
                    cv::Mat transformed(height, width, CV_8UC1);
                    cv::warpPerspective(cam_man.image_depth_cv, transformed,
                                        cam_man.homography,
                                        cv::Size(width, height));
                    // imshow(window_name, transformed);
                    image = transformed;

                } catch (const std::exception &e) {
                    std::cerr << "Image processing failed; " << e.what()
                              << '\n';
                }
            }

            if (m_state.process) postProcessing(image);

            m_state.key = cv::waitKey(10);

            m_state.action();
        }
    }

    void postProcessing(cv::Mat image) {
        try {
            // TODO color coding for objects via tamplates
            m_image_processor.getImage(&image);

            for (auto action : m_settings.erodil) {
                if (action.type == ErosionDilation::Dilation)
                    m_image_processor.dilate(action.distance, action.size);
                else
                    m_image_processor.erode(action.distance, action.size);
            }

            m_image_processor.setParameteresFromSettings(m_settings);
            m_image_processor.findObjects();
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
            m_state.load_settings = false;
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
            // image_processer.update(erodil);
            // image_processer.update(settings)
            m_state.load_settings = false;
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
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
            std::cerr << e.what() << '\n';
            m_state.restart_cam = false;
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

    Loop loop(settings, printer, logger, image_processor);
    loop.Process();
    return EXIT_SUCCESS;
}

void signalHandler(int signalNumber) {
    if (signalNumber == SIGTERM || signalNumber == SIGHUP) {
        std::cout << "Recieved interupt signal: " << signalNumber
                  << "\nExiting..." << std::endl;
        exit(signalNumber);
    }
}
