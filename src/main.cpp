

#include "opencv2/opencv.hpp"
#include <fstream>
#include <vector>

using namespace cv;
using namespace std;

int main(int argc, char **argv) {

    if (argc <= 1) {
        cout << "Usage: \n";
        cout << "$ ZED_SVO_Playback <SVO_file> \n";
        cout << "  ** SVO file is mandatory in the application ** \n\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
