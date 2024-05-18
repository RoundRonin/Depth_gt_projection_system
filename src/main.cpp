
#include "../include/sl_utils.hpp"
#include "converter.hpp"
#include "object_recognition.cpp"
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

struct Settings {
    bool from_SVO = false;

    bool save_logs = false;
    bool measure_time = false;
    char recurse = false;
    char debug_level = 0;

    uchar zlimit = 10;
    uchar minDistance = 0;
    uchar medium_limit = 10;
    int minArea = 1000;
    int maxObjects = 10;

    string FilePath;
    string OutputLocation = "./Result/";
};

int main(int argc, char **argv) {

    Settings sets{};

    if (argc <= 1) {
        cout << "Usage: \n";
        cout << "$ " << argv[0] << " <DEPTH_MAP> \n";
        cout << "or\n";
        cout << "$ " << argv[0] << " <SVO> \n";
        cout << "  ** Depth map file or .SVO file is mandatory in the "
                "application ** \n\n";
        return EXIT_FAILURE;
    }

    string filename = argv[1];
    if (filename.substr(filename.size() - 4) == ".svo") {
        sets.from_SVO = true;
    }

    if (argv[1][0] == '-')
        return EXIT_FAILURE;

    sets.FilePath = argv[1];

    // TODO fix crash when "-A 10" with a space inbetween
    for (int i = 2; i < argc; i++) {
        char *arg = argv[i];
        vector<char> flags = {};
        if (arg[0] == '-') {
            int j = 1;
            while (arg[j] > 64 && arg[j] < 91 || arg[j] > 96 && arg[j] < 123) {
                flags.push_back(arg[j]);
                j++;
            }
        } else
            continue;

        bool wasArgWithParams = false;

        for (auto flag : flags) {
            switch (flag) {
            case 'l': { // turn on log saving
                sets.save_logs = true;
                break;
            }
            case 't': { // turn on measuring time
                sets.measure_time = true;
                break;
            }
            case 'r': { // use recursion instead of iterations
                sets.recurse = true;
                break;
            }
            case 'd': { // set Debug level
                if (wasArgWithParams)
                    break;

                char level = atoi(argv[i + 1]);
                if (level < 0)
                    level = 0;
                if (level > 2)
                    level = 2;

                sets.debug_level = level;
                wasArgWithParams = true;
                break;
            }
            case 'S': { // set output location
                if (wasArgWithParams)
                    break;

                sets.OutputLocation = argv[i + 1];
                wasArgWithParams = true;
                break;
            }
            case 'Z': { // depth difference limit
                if (wasArgWithParams)
                    break;

                uchar zlimit = atoi(argv[i + 1]);
                // TODO checks;
                sets.zlimit = zlimit;
                wasArgWithParams = true;
                break;
            }
            case 'D': { // min distance
                if (wasArgWithParams)
                    break;

                uchar minDistance = atoi(argv[i + 1]);
                // TODO checks;
                sets.minDistance = minDistance;
                wasArgWithParams = true;
                break;
            }
            case 'M': { // medium
                if (wasArgWithParams)
                    break;

                uchar medium = atoi(argv[i + 1]);
                // TODO checks;
                sets.medium_limit = medium;
                wasArgWithParams = true;
                break;
            }
            case 'A': { // min area
                if (wasArgWithParams)
                    break;

                int minArea = atoi(argv[i + 1]);
                // TODO checks;
                sets.minArea = minArea;
                wasArgWithParams = true;
                break;
            }
            case 'O': { // max objects
                if (wasArgWithParams)
                    break;

                int maxObjects = atoi(argv[i + 1]);
                // TODO checks;
                sets.maxObjects = maxObjects;
                wasArgWithParams = true;
                break;
            }
            default:
                break;
            }
        }
    }

    Logger logger("log", 0, 0, sets.save_logs, sets.measure_time,
                  sets.debug_level);

    Image image;

    if (sets.from_SVO) {
        CameraManager camMan(argv[1]);
        camMan.openCamera();

        camMan.initSVO();
        auto returned_state = camMan.grab();
        if (returned_state <= ERROR_CODE::SUCCESS) {
            auto result = camMan.imageProcessing();
            cv::Mat img = slMat2cvMat(result.second);
            image = Image(img, sets.OutputLocation, logger);
        }
    } else {
        image = Image(sets.FilePath, sets.OutputLocation, logger);
    }

    image.write(sets.OutputLocation + "init_image.png");

    image.erode(5, 5);
    image.dilate(5, 5);

    image.erode(3, 3);
    image.dilate(3, 3);

    image.erode(2, 2);
    image.dilate(2, 2);

    image.dilate(3, 3);

    image.write(sets.OutputLocation + "modified_image.png");
    image.findObjects(sets.zlimit, sets.minDistance, sets.medium_limit,
                      sets.minArea, sets.maxObjects, sets.recurse);

    int width = image.image.cols;
    int height = image.image.rows;
    cv::Size size = image.image.size();

    // VideoWriter video(OutputLocation + "color_gradient.avi",
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
