
#include "object_recognition.cpp"
#include "templategen.cpp"

using namespace cv;
using namespace std;

struct Settings {
    bool save_logs = false;
    bool measure_time = false;
    char recurse = false;
    char debug_level = 0;

    uchar zlimit = 10;
    uchar minDistance = 0;
    int minArea = 1000;
    int maxObjects = 10;

    string FilePath;
    string OutputLocation = "./Result/";
};

int main(int argc, char **argv) {

    Settings settings{};

    if (argc <= 1) {
        cout << "Usage: \n";
        cout << "$ " << argv[0] << " <DEPTH_MAP> \n";
        cout << "  ** Depth map file is mandatory in the application ** \n\n";
        return EXIT_FAILURE;
    }

    if (argv[1][0] == '-')
        return EXIT_FAILURE;

    settings.FilePath = argv[1];

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
                settings.save_logs = true;
                break;
            }
            case 't': { // turn on measuring time
                settings.measure_time = true;
                break;
            }
            case 'r': { // use recursion instead of iterations
                settings.recurse = true;
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

                settings.debug_level = level;
                wasArgWithParams = true;
                break;
            }
            case 'S': { // set output location
                if (wasArgWithParams)
                    break;

                settings.OutputLocation = argv[i + 1];
                wasArgWithParams = true;
                break;
            }
            case 'Z': { // depth difference limit
                if (wasArgWithParams)
                    break;

                char zlimit = atoi(argv[i + 1]);
                // TODO checks;
                settings.zlimit = zlimit;
                wasArgWithParams = true;
                break;
            }
            case 'D': { // min distance
                if (wasArgWithParams)
                    break;

                char minDistance = atoi(argv[i + 1]);
                // TODO checks;
                settings.minDistance = minDistance;
                wasArgWithParams = true;
                break;
            }
            case 'A': { // min area
                if (wasArgWithParams)
                    break;

                int minArea = atoi(argv[i + 1]);
                // TODO checks;
                settings.minArea = minArea;
                wasArgWithParams = true;
                break;
            }
            case 'O': { // max objects
                if (wasArgWithParams)
                    break;

                int maxObjects = atoi(argv[i + 1]);
                // TODO checks;
                settings.maxObjects = maxObjects;
                wasArgWithParams = true;
                break;
            }
            default:
                break;
            }
        }
    }
    // uchar zlimit = 10, uchar minDistance = 0,
    //                      int minDots = 1000, int maxObjects = 5,
    Image image(settings.FilePath, settings.OutputLocation);

    image.erode(5, 5);
    image.dilate(5, 5);

    image.erode(3, 3);
    image.dilate(3, 3);

    image.erode(2, 2);
    image.dilate(2, 2);

    image.dilate(3, 3);

    image.write(settings.OutputLocation + "modified_image.png");
    image.findObjects(settings.zlimit, settings.minDistance, settings.minArea,
                      settings.maxObjects);

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
