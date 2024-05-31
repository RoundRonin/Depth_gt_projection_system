#include <gtest/gtest.h>

#include <opencv2/core/mat.hpp>
#include <opencv2/core/matx.hpp>
#include <opencv2/core/types.hpp>

#include "../src/headers/object_recognition.hpp"
#include "../src/headers/utils.hpp"

double getPointToPlaneDistance(cv::Vec3d plane_point, cv::Vec3d plane_vector,
                               cv::Vec3d point) {
    auto dot =
        plane_vector.dot({point[0] - plane_point[0], point[1] - plane_point[1],
                          point[2] - plane_point[2]});

    auto length = cv::norm(plane_vector);
    return (std::abs(dot) / length);
}

cv::Point getClosestPoint(cv::Mat &image, cv::Point point) {
    std::vector<cv::Point> pointsToCheck = {};
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            pointsToCheck.push_back({i, j});
        }
    }
}

// cv::Vec3d getPointVector(cv::Mat &image, cv::Vec3d point_new,
//                          cv::Vec3d point_old) {
void getPointVector() {
    cv::Vec3d vec(1, 2, 3);
    cv::Vec3d point(1, -1, 0);

    // Calculate the perpendicular vector
    cv::Matx33d rotationMat = cv::Matx33d::eye();
    cv::Vec3d zAxis = {0, 0, 1};

    // if (norm(vec.cross(zAxis)) > 1e-6) {
    //     rotationMat = cv::Matx33d::eye().cross(-zAxis * vec).t();
    // }

    cv::Vec3d perpendicular_vec = rotationMat * zAxis;

    // Normalize the vector to make it unit vector
    perpendicular_vec = perpendicular_vec / norm(perpendicular_vec);

    // Print the calculated perpendicular unit vector
    std::cout << "Perpendicular Unit Vector: [" << perpendicular_vec[0] << ", "
              << perpendicular_vec[1] << ", " << perpendicular_vec[2] << "]"
              << std::endl;
}

TEST(VectorSuit, Distance) {
    double dist = getPointToPlaneDistance({0, 0, 0}, {0, 0, 1}, {0, 0, 5});
    EXPECT_EQ(5, dist);
    double dist2 = getPointToPlaneDistance({1, 1, 1}, {5, 6, 1}, {7, 4, 5});
    EXPECT_NEAR(6.6, dist2, 0.05);
    double dist3 = getPointToPlaneDistance({1, 1, 1}, {5, 6, 1}, {1, 1, 1});
    EXPECT_EQ(0, dist3);
    // double dist4 = getPointToPlaneDistance({0, 0, 0}, {0, 0, 0}, {0, 0,
    // 0}); EXPECT_EQ(nan, dist4);
}

TEST(VectorSuit, Closeness) {
    getPointVector();

    // double dist4 = getPointToPlaneDistance({0, 0, 0}, {0, 0, 0}, {0, 0,
    // 0}); EXPECT_EQ(nan, dist4);
}