
#include "../include/sl_utils.hpp"
#include "converter.hpp"
#include "object_recognition.cpp"
#include "settings.hpp"
#include "templategen.cpp"
#include "utils.cpp"

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

    printer.setDebugLevel(settings.debug_level);
    Logger logger("log", 0, 0, settings.save_logs, settings.measure_time,
                  settings.debug_level);
    Image image;

    if (settings.from_SVO) {
        CameraManager camMan((settings.FilePath).c_str());
        camMan.openCamera();

        camMan.initSVO();
        auto returned_state = camMan.grab();
        if (returned_state <= ERROR_CODE::SUCCESS) {
            auto result = camMan.imageProcessing();
            // cv::Mat img = slMat2cvMat(result.second);
            result.second.write(
                (settings.outputLocation.value + "init_image.png").c_str());
            // image1 = Image(img, sets.outputLocation, logger);
            image = Image(settings.outputLocation.value + "init_image.png",
                          settings.outputLocation.value, logger, printer);
        }
    } else {
        image = Image(settings.FilePath, settings.outputLocation.value, logger,
                      printer);
    }

    // TODO romeve this workaround. It currently saves an image
    // TODO and then reads it.
    // TODO it seems, slMat2cvMat has some kind of a bug
    // image.write(settings.outputLocation.value + "init_image.png");

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

    return EXIT_SUCCESS;
}
