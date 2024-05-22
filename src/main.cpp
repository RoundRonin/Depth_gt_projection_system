
#include "../include/sl_utils.hpp"
#include "./headers/converter.hpp"
#include "./headers/settings.hpp"
#include "./impl/object_recognition.cpp"
#include "./impl/templategen.cpp"
#include "./impl/utils.cpp"

using namespace cv;
using namespace std;
using namespace dm;

// TODO check if an image exists in the location
// TODO frontend
// TODO     show log statistics
// TODO
// TODO backend
// TODO     save test image in a "database"
// TODO
// TODO

// TODO free cam on process kill

// TODO use zed.setRegionOfInterest to discard all area beyond the projector
void process(Logger &logger, CameraManager &camMan, Settings &settings,
             ImageProcessor &image) {
    logger.start();
    auto returned_state = camMan.grab();
    if (returned_state <= ERROR_CODE::SUCCESS) {
        auto result = camMan.imageProcessing();

        // cv::Mat img = slMat2cvMat(result.second);
        auto state = result.second.write(
            (settings.outputLocation.value + "init_image.png").c_str());
        // image1 = ImageProcessor(img, sets.outputLocation, logger);
        std::cerr << state << std::endl;
        image.getImage(settings.outputLocation.value + "init_image.png");
    } else {
        throw runtime_error("Coudn't grab a frame");
    }
    logger.stop("Zed grab");
    logger.print();
}

void createVideo() {
    // VideoWriter video(outputLocation + "color_gradient.avi",
    //                   VideoWriter::fourcc('M', 'J', 'P', 'G'), 60, size);

    // if (!video.isOpened()) {
    //     return -1;
    // }

    // vector<cv::Mat> masks = image.mask_mats;
    // Templates templates(image.image);
    // for (int i = 0; i < 10 * 60; i++) { // 10 seconds at 60 fps

    //     vector<cv::Mat> results;
    //     results.push_back(templates.chessBoard(i, masks.at(1), 2, 1));
    //     results.push_back(templates.chessBoard(i, masks.at(2), 1, 3));
    //     results.push_back(templates.gradient(i, masks.at(3), 1));
    //     results.push_back(templates.gradient(i, masks.at(4), 5));

    //     cv::add(results.at(0), results.at(1), results.at(1));
    //     cv::add(results.at(1), results.at(2), results.at(2));
    //     cv::add(results.at(2), results.at(3), results.at(3));

    //     video.write(results.at(3));
    // }

    // video.release();
}

int main(int argc, char **argv) {
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

    printer.setDebugLevel(
        static_cast<Printer::DEBUG_LVL>(settings.debug_level));

    Logger logger("log", 0, 0, settings.save_logs, settings.measure_time,
                  settings.debug_level);

    ImageProcessor image(settings.outputLocation.value, logger, printer);
    if (settings.type == Settings::SOURCE_TYPE::IMAGE) {
        // image = ImageProcessor(settings.FilePath,
        // settings.outputLocation.value,
        //                        logger, printer);
        image.getImage(settings.FilePath);
    } else {
        CameraManager camMan;
        if (settings.type == Settings::SOURCE_TYPE::SVO)
            // does it even work?
            camMan.initSVO((settings.FilePath).c_str());

        int dm = settings.depth_mode;
        CameraManager::Params params{
            .depth_mode =
                static_cast<sl::DEPTH_MODE>(settings.depth_mode.value),
            .fill_mode = settings.fill_mode,
            .max_distance = settings.camera_distance,
            .texture_threshold = settings.texture_threshold,
            .threshold = settings.threshold};

        // CameraManager::Params params(
        //         static_cast<sl::DEPTH_MODE>(settings.depth_mode.value),

        camMan.setParams(params);
        camMan.openCamera();

        process(logger, camMan, settings, image);
    }

    // TODO romeve this workaround. It currently saves an image
    // TODO and then reads it.
    // TODO it seems, slMat2cvMat has some kind of a bug
    // image.write(settings.outputLocation.value + "init_image.png");

    // TODO mainloop
    // ? multithreaded? One for video creation, one for showing and one for the
    // server? while (1) {
    // }

    // TODO 5544332244
    image.dilate(5, 5);
    image.erode(5, 5);

    image.write(settings.outputLocation.value + "modified_image.png");
    image.findObjects(settings.zlimit, settings.minDistance,
                      settings.medium_limit, settings.minArea,
                      settings.maxObjects, settings.recurse);

    int width = image.image.cols;
    int height = image.image.rows;
    cv::Size size = image.image.size();

    // Main loop: show frame by frame generated video images (via
    // templategen function)
    //  But start with a calibration: show an image of checkerboard
    //  Then read it with camera and understand an area of operations from
    //  that
    //  -- get an image of the board and form all the neccacery spatial
    //  transitions for a resulting vido to be projected (fish-eye, spatial
    //  rotation)
    //
    //

    return EXIT_SUCCESS;
}
