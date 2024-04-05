

#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/opencv.hpp"
#include <fstream>
#include <vector>

using namespace cv;
using namespace std;

class Image {
    cv::Mat image;
    cv::Mat objects;

  public:
    Image(string path) {
        image = cv::imread(path, IMREAD_GRAYSCALE);
        objects = cv::Mat(image.size(), image.type());
    }

    void write(string path) { cv::imwrite(path, image); }

    cv::Mat erode(int erosion_dst, int erosion_size) {
        int erosion_type = MORPH_ELLIPSE;
        Mat element = getStructuringElement(
            erosion_type, Size(2 * erosion_size + 1, 2 * erosion_size + 1),
            Point(erosion_size, erosion_size));

        cv::Mat output;
        cv::erode(image, output, element);

        image = output;
        return output;
    }

    cv::Mat dilate(int dilation_dst, int dilation_size) {
        int dilation_type = MORPH_ELLIPSE;
        Mat element = getStructuringElement(
            dilation_type, Size(2 * dilation_size + 1, 2 * dilation_size + 1),
            Point(dilation_size, dilation_size));

        cv::Mat output;
        cv::dilate(image, output, element);

        image = output;
        return output;
    }

    void findObjects(uchar z_limit = 10) {
        cout << "using z_limit = " << (int)z_limit << endl;
        if (image.empty())
            return;

        CV_Assert(image.depth() == CV_8U);
        int nRows = image.rows;
        int nCols = image.cols;

        for (int i = 0; i < nRows; i++) {
            for (int j = 0; j < nCols; j++) {
                uchar val = image.at<uchar>(i, j);

                if (val == 0)
                    continue;
                if (objects.at<uchar>(i, j) != 0)
                    continue;

                cv::Point current(j, i);

                uchar id = 100;
                paint(image, objects, z_limit, current, id);
            }
        }

        imwrite("objects.png", objects);
    }

  private:
    int directions[4][2] = {{1, 0}, {0, 1}, {0, -1}, {-1, 0}};

    bool walk(cv::Mat &image, cv::Mat &objects, uchar z_limit, uchar prev_z,
              cv::Point current, vector<cv::Point> path, uchar id) {
        // Base case:
        // 1. Pixel is off the map
        if (!image.at<uchar>(current)) //!
            return false;

        // 2. Pixel is black
        if (image.at<uchar>(current) == 0)
            return false;

        // 3. Pixel is in the objects
        if (objects.at<uchar>(current) != 0)
            return false;

        // 4. z_limit exceeded
        if (abs(image.at<uchar>(current) - prev_z) > z_limit)
            return false;

        objects.at<uchar>(current) = id; // Painting the pixel

        path.push_back(current);
        // recurse
        for (int i = 0; i < 4; i++) {
            int x = directions[i][0];
            int y = directions[i][1];

            cv::Point next(y, x);
            uchar current_z = image.at<uchar>(current);
            walk(image, objects, z_limit, current_z, next, path, id);
        }

        if (path.size() == 0)
            return false;
        path.pop_back();
    }
    void paint(cv::Mat &image, cv::Mat &objects, int z_limit, cv::Point start,
               uchar id) {
        vector<cv::Point> path;
        uchar start_z = image.at<uchar>(start);

        walk(image, objects, z_limit, start_z, start, path, id);
    }
};

int main(int argc, char **argv) {

    if (argc <= 1) {
        cout << "Usage: \n";
        cout << "$ " << argv[0] << " <DEPTH_MAP> \n";
        cout << "  ** Depth map file is mandatory in the application ** \n\n";
        return EXIT_FAILURE;
    }

    Image image(argv[1]);

    image.erode(5, 5);
    image.write("e.png");
    image.dilate(5, 5);
    image.write("e_d.png");

    image.erode(3, 3);
    image.dilate(3, 3);

    image.erode(2, 2);
    image.dilate(2, 2);

    image.write("e_d_e_d_e_d.png");
    image.findObjects(atoi(argv[2]));

    return EXIT_SUCCESS;
}
