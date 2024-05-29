
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

int main(int argc, char **argv) {
    // TODO add more signals, work on handling
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);

    Settings settings;
    Printer printer;

    auto Result = settings.Init(argc, argv);
    if (Result == Printer::ERROR::ARGS_FAILURE) {
        printer.log_message({Printer::ERROR::ARGS_FAILURE, {}, argv[0]});
        return EXIT_FAILURE;
    }

    try {
        settings.Parse();
    } catch (const std::exception &x) {
        printer.log_message({Printer::ERROR::FLAGS_FAILURE, {}, argv[0]});
        printer.log_message(x);
        return EXIT_FAILURE;
    }

    Config config = settings.config;
    vector<ErosionDilation> erodil = settings.erodil;
    HoughLinesPsets hough_params = settings.hough_params;

    printer.setDebugLevel(static_cast<Printer::DEBUG_LVL>(config.debug_level));

    Logger logger("log", 0, 0, config.save_logs, config.maesure_time,
                  config.debug_level);

    ImageProcessor image(config.output_location, logger, printer);
    if (config.type == Config::SOURCE_TYPE::IMAGE) {
        image.getImage(config.file_path);
    } else {
        zed::CameraManager camMan(printer);

        try {
            camMan.openCam(config);
            // camMan.imageProcessing(false);
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
        }

        InteractiveState state;
        state.printHelp();
        string window_name = "Projection";
        cv::namedWindow(window_name, cv::WindowFlags::WINDOW_NORMAL);
        cv::setWindowProperty(window_name,
                              cv::WindowPropertyFlags::WND_PROP_FULLSCREEN,
                              cv::WindowFlags::WINDOW_FULLSCREEN);
        // Templates templates(image.image);
        state.calibrate = true;
        while (state.key != 'q') {
            // ? multithreaded? One video creation, one showing and one server
            state.next = 0;
            state.idx = 0;

            if (state.load_settings) {
                settings.ParseConfig();
                config = settings.config;
                erodil = settings.erodil;
                hough_params = settings.hough_params;

                camMan.updateRunParams(config);
                camMan.updateHough(hough_params);
                state.load_settings = false;
            }

            if (state.calibrate) {
                try {
                    camMan.calibrate(window_name, config.output_location,
                                     15000);
                    state.calibrate = false;
                } catch (const std::exception &e) {
                    std::cerr << "Calibration failed; " << e.what() << '\n';
                }
            }

            std::cerr << "hello" << std::endl;
            auto returned_state = camMan.grab();
            if (returned_state == sl::ERROR_CODE::SUCCESS) {
                try {
                    camMan.imageProcessing(false);
                    // cv::Mat ROI =
                    // camMan.image_depth_cv(camMan.image_mask_cv);
                    // imshow(window_name, camMan.image_depth_cv);
                    int height = camMan.image_depth_cv.rows;
                    int width = camMan.image_depth_cv.cols;
                    cv::Mat transformed(height, width, CV_8UC1);
                    cv::warpPerspective(camMan.image_depth_cv, transformed,
                                        camMan.homography,
                                        cv::Size(width, height));
                    imshow(window_name, transformed);
                } catch (const std::exception &e) {
                    std::cerr << "Image processing failed; " << e.what()
                              << '\n';
                }
            }

            state.key = cv::waitKey(10);

            // TODO color coding for objects via tamplates

            state.action();
            // ! calibration
        }

        camMan.~CameraManager();
    }

    return EXIT_SUCCESS;
}

void signalHandler(int signalNumber) {
    if (signalNumber == SIGTERM || signalNumber == SIGHUP) {
        std::cout << "Recieved interupt signal: " << signalNumber
                  << "\nExiting..." << std::endl;
        exit(signalNumber);
    }
}
