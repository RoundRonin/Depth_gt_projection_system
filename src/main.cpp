
#include "object_recognition.cpp"
#include "templategen.cpp"

using namespace cv;
using namespace std;

int main(int argc, char **argv) {

    string OutputLocation = "./Result/";

    if (argc <= 1) {
        cout << "Usage: \n";
        cout << "$ " << argv[0] << " <DEPTH_MAP> \n";
        cout << "  ** Depth map file is mandatory in the application ** \n\n";
        return EXIT_FAILURE;
    }

    std::cout << argv[1] << std::endl;
    Image image(argv[1], OutputLocation);

    image.erode(5, 5);
    image.dilate(5, 5);

    image.erode(3, 3);
    image.dilate(3, 3);

    image.erode(2, 2);
    image.dilate(2, 2);

    image.dilate(3, 3);

    image.write(OutputLocation + "e_d_e_d_e_d_d.png");
    image.findObjects(atoi(argv[2]), 1000, 10);

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
