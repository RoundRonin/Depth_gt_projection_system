
#include "opencv2/opencv.hpp"
#include <fstream>
#include <sl/Camera.hpp>
#include <vector>
// #include "/mnt/jetson_root/usr/local/zed"
using namespace cv;
using namespace sl;
using namespace std;

namespace dm {

void printTimeTaken(double start, double end, string function_name) {
    double time_taken = double(end - start) / double(CLOCKS_PER_SEC);
    cout << "Time taken by " + function_name + " is: " << fixed << time_taken
         << setprecision(5);
    cout << " sec " << endl;
}

void print(string msg_prefix, ERROR_CODE err_code, string msg_suffix) {
    cout << "[Sample]";
    if (err_code != ERROR_CODE::SUCCESS)
        cout << "[Error] ";
    else
        cout << " ";
    cout << msg_prefix << " ";
    if (err_code != ERROR_CODE::SUCCESS) {
        cout << " | " << toString(err_code) << " : ";
        cout << toVerbose(err_code);
    }
    if (!msg_suffix.empty())
        cout << " " << msg_suffix;
    cout << endl;
}

// template <typename T> struct Parameter {
//     T value;
//     option opt;
//     string help;
// };

struct spatial_data {
    vector<sl::float4> point_cloud_values;
    vector<array<int, 3>> triangles;
};

struct cam_matrix {
    float cx;
    float cy;
    float fx;
    float fy;
};

class CameraManager {
  private:
    Camera zed;
    InitParameters init_parameters;

    sl::Mat image;
    sl::Mat image_depth;
    sl::Mat depth_map;
    sl::Mat generated_point_cloud;
    spatial_data mesh;

    sl::Resolution resolution;

    RuntimeParameters runParameters;
    cam_matrix cam;

    int svo_position;

  public:
    CameraManager(sl::String filepath,
                  DEPTH_MODE depth_mode = DEPTH_MODE::ULTRA,
                  int max_distance = 20) {
        // Create ZED objects
        init_parameters.input.setFromSVOFile(filepath);

        init_parameters.depth_mode = depth_mode;
        init_parameters.coordinate_system =
            COORDINATE_SYSTEM::RIGHT_HANDED_Y_UP;
        init_parameters.sdk_verbose = 1; // TODO cli?

        init_parameters.camera_resolution = RESOLUTION::AUTO;
        init_parameters.coordinate_units = UNIT::METER;
        init_parameters.depth_maximum_distance = max_distance;
    }

    void setParams(int threshold = 10, int texture_threshold = 100,
                   bool fill_mode = false) {
        runParameters.confidence_threshold = threshold;
        runParameters.texture_confidence_threshold = texture_threshold;
        runParameters.enable_fill_mode = fill_mode;
    }

    ERROR_CODE openCamera() {
        // Open the camera
        auto returned_state = zed.open(init_parameters);
        if (returned_state != ERROR_CODE::SUCCESS) {
            print("Camera Open", returned_state, "Exit program.");
            return ERROR_CODE::FAILURE;
        }

        resolution = zed.getCameraInformation().camera_configuration.resolution;

        auto params = zed.getCameraInformation()
                          .camera_configuration.calibration_parameters.left_cam;

        cam = cam_matrix{params.cx, params.cy, params.fx, params.fy};

        return ERROR_CODE::SUCCESS;
    }

    void initSVO() {
        int nb_frames = zed.getSVONumberOfFrames();
        svo_position = (int)nb_frames / 2; // TODO cli??

        print("[Info] SVO contains " + to_string(nb_frames) + " frames",
              ERROR_CODE::SUCCESS, "");

        print("[Info] SVO position: frame " + to_string(svo_position),
              ERROR_CODE::SUCCESS, "");

        zed.setSVOPosition(svo_position);
    }

    ERROR_CODE grab() { return zed.grab(runParameters); }

    std::pair<sl::Mat, sl::Mat> imageProcessing(bool write = false) {
        zed.retrieveImage(image, VIEW::LEFT, MEM::CPU, resolution);
        zed.retrieveImage(image_depth, VIEW::DEPTH);

        if (write) {
            if (image.write(
                    ("capture_" + to_string(svo_position) + ".png").c_str()) ==
                ERROR_CODE::SUCCESS)
                cout << "image file saving succeded" << endl;
            if (image_depth.write(
                    ("capture_depth_" + to_string(svo_position) + ".png")
                        .c_str()) == ERROR_CODE::SUCCESS)
                cout << "depth image file saving succeded" << endl;
        }

        return {image, image_depth};
    }

    void depthMapProcessing() {
        zed.retrieveMeasure(depth_map, MEASURE::DEPTH);
    }

    void pointCloudProcessing(bool write = false) {
        generated_point_cloud = formPointCloud(depth_map, image, cam);
        mesh = formMesh(generated_point_cloud);

        if (write) {
            ERROR_CODE status = savePly("Mesh.ply", mesh);
            if (status == ERROR_CODE::SUCCESS)
                cout << "My mesh .ply file saving succeded" << endl;
            else
                cout << "My mesh .ply file saving failed" << endl;
        }
    }

    ~CameraManager() { zed.close(); }

  private:
    sl::Mat formPointCloud(sl::Mat depth_map, sl::Mat image, cam_matrix cam) {
        // TODO add coordinate system options
        // TODO check if image is present, if cam is present and provide
        // defaults

        int height = depth_map.getHeight();
        int width = depth_map.getWidth();
        sl::Mat generated_point_cloud(width, height, MAT_TYPE::F32_C4,
                                      MEM::CPU);

        float depth;
        sl::float4 point;
        sl::char4 color;
        sl::uchar4 colorData;
        unsigned char *bytePointer;
        float colorFloat;

        bytePointer = reinterpret_cast<unsigned char *>(&colorFloat);

        for (int row = 0; row < height; row++) {
            for (int column = 0; column < width; column++) {
                depth_map.getValue(column, row, &depth);
                image.getValue(column, row, &color);

                point.z = -depth;
                point.x = (column - cam.cx) * depth / cam.fx;
                point.y = -(row - cam.cy) * depth / cam.fy;

                bytePointer[0] = color.b;
                bytePointer[1] = color.g;
                bytePointer[2] = color.r;
                bytePointer[3] = color.a;

                point.w = colorFloat;

                generated_point_cloud.setValue(column, row, point);
            }
        }

        return generated_point_cloud;
    }

    spatial_data cleanPointCloud(sl::Mat pointcloud) {
        // Discards non finite points

        vector<sl::float4> point_cloud_values;
        int vertex_amount = 0;
        sl::float4 value;

        int width = pointcloud.getWidth();
        int height = pointcloud.getHeight();

        for (int column = 0; column < width; column++) {
            for (int row = 0; row < height; row++) {
                pointcloud.getValue(column, row, &value);

                if (isfinite(value.x)) {
                    point_cloud_values.push_back(value);
                }
            }
        }

        return {point_cloud_values, {}};
    }

    spatial_data formMesh(sl::Mat pointcloud) {

        vector<sl::float4> point_cloud_values;
        vector<array<int, 3>> triangles;

        sl::float4 potential_vertecies[4];
        sl::float4 point;

        int vertex_amount = 0;
        int faces_amount = 0;

        int width = pointcloud.getWidth();
        int height = pointcloud.getHeight();

        struct triangle_vertex {
            int position;
            unsigned char vertex;
        };

        // Containes all references to triangles that contain a point
        vector<vector<triangle_vertex>> triangle_reference(width * height);

        // Define all point-triangle references (both directions -- performance)
        for (int row = 0; row < height - 1; row++) {
            for (int column = 0; column < width - 1; column++) {

                pointcloud.getValue(column, row,
                                    &potential_vertecies[0]); // v0 v0\  /v1
                pointcloud.getValue(
                    column + 1, row,
                    &potential_vertecies[1]); // v1    0   0  1  2  .  .  . n-1
                pointcloud.getValue(
                    column, row + 1,
                    &potential_vertecies[2]); // v2    1   n n+1 .  .  .  . n-2
                pointcloud.getValue(column + 1, row + 1,
                                    &potential_vertecies[3]); // v3    . v2/ \v3

                int v0 = row * width + column;
                int v1 = row * width + column + 1;
                int v2 = (row + 1) * width + column;
                int v3 = (row + 1) * width + column + 1;

                //
                // Upper-left triangle
                //

                bool valid_triangle = true;

                for (int point_number = 0; point_number < 3; point_number++) {
                    point = potential_vertecies[point_number];

                    // check if this particular vertex is NaN, infinite etc.
                    valid_triangle = valid_triangle && isfinite(point.x) &&
                                     isfinite(point.y) && isfinite(point.z);
                }

                if (valid_triangle) {
                    triangles.push_back({v0, v1, v2});

                    int last_index = triangles.size() - 1;
                    // Push last triangle index and position of the point in the
                    // triangle to all three points that it contains
                    triangle_reference.at(v0).push_back({last_index, 0});
                    triangle_reference.at(v1).push_back({last_index, 1});
                    triangle_reference.at(v2).push_back({last_index, 2});
                }

                //
                // Lower-right triangle
                //

                valid_triangle = true;

                for (int point_number = 1; point_number < 4; point_number++) {
                    point = potential_vertecies[point_number];

                    // check if this particular vertex is NaN, infinite etc.
                    valid_triangle = valid_triangle && isfinite(point.x) &&
                                     isfinite(point.y) && isfinite(point.z);
                }

                if (valid_triangle) {
                    triangles.push_back({v1, v2, v3});

                    int last_index = triangles.size() - 1;
                    // Push last triangle index and position of the point in the
                    // triangle to all three points that it contains
                    triangle_reference.at(v1).push_back({last_index, 0});
                    triangle_reference.at(v2).push_back({last_index, 1});
                    triangle_reference.at(v3).push_back({last_index, 2});
                }
            }
        }

        // Point cloud values filtration (final set of vertecies prepapation)
        for (int row = 0; row < height; row++) {
            for (int column = 0; column < width; column++) {
                pointcloud.getValue(column, row, &point);
                int current_index = row * width + column;

                if (triangle_reference.at(current_index).size() > 0) {
                    point_cloud_values.push_back(point);
                    int last_saved_index = point_cloud_values.size() - 1;

                    // Remap all the triangles' point indecies to reference this
                    // new point_cloud_values vector
                    //
                    // This solution is the reason to use bidirecitonal
                    // reference earlier: it allows prevention of an additional
                    // time-conuming lookup operation (it would add at least
                    // O(n) on top of everything worsening the overall time
                    // complexity from O(n) to O(n^2))
                    //
                    for (const auto &triangle :
                         triangle_reference.at(current_index)) {
                        triangles.at(triangle.position).at(triangle.vertex) =
                            last_saved_index;
                    }
                }
            }
        }

        return {point_cloud_values, triangles};
    }

    ERROR_CODE savePly(string filename, spatial_data data) {
        ofstream file(filename);
        sl::uchar4 colorData;
        unsigned char *bytePointer;
        float colorFloat;

        int vertex_amount = data.point_cloud_values.size();
        int faces_amount = data.triangles.size();

        if (vertex_amount == 0)
            return ERROR_CODE::FAILURE;

        file << "ply\n";
        file << "format ascii 1.0\n";
        file << "element vertex " << vertex_amount << "\n";
        file << "property float x\n";
        file << "property float y\n";
        file << "property float z\n";
        file << "property uchar red\n";
        file << "property uchar green\n";
        file << "property uchar blue\n";
        if (faces_amount > 0) {
            file << "element face " << faces_amount << "\n";
            file << "property list uchar int vertex_indices\n";
        }
        file << "end_header\n";

        for (const auto &value : data.point_cloud_values) {
            colorFloat = value.w;

            bytePointer = reinterpret_cast<unsigned char *>(&colorFloat);

            colorData.r = bytePointer[0];
            colorData.g = bytePointer[1];
            colorData.b = bytePointer[2];
            // colorData.a = bytePointer[3]; // alpha channel unused

            // TODO optimisation needed (char conversion takes considerable
            // amout of time)
            file << value.x << " " << value.y << " " << value.z << " "
                 << +colorData.r << " " << +colorData.g << " " << +colorData.b
                 << "\n";
        }

        if (faces_amount > 0)
            for (const auto &face : data.triangles) {
                file << 3 << " " << face[0] << " " << face[1] << " " << face[2]
                     << "\n";
            }

        file.close();

        return ERROR_CODE::SUCCESS;
    }
};

}; // namespace dm
