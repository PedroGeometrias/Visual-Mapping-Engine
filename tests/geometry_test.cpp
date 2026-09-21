#include "geometry/homography.hpp"
#include "matching/ransac.hpp"

#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <vector>

static bool matrix_close(
    Eigen::Matrix3d actual,
    Eigen::Matrix3d expected,
    double tolerance
){
    if(std::abs(actual(2, 2)) > 1e-12){
        actual /= actual(2, 2);
    }

    if(std::abs(expected(2, 2)) > 1e-12){
        expected /= expected(2, 2);
    }

    return (actual - expected).cwiseAbs().maxCoeff() < tolerance;
}

static bool test_homography(
    const std::string& name,
    const Eigen::Matrix3d& expected
){
    const std::vector<Eigen::Vector2d> source = {
        {10.0, 20.0},
        {180.0, 30.0},
        {25.0, 160.0},
        {200.0, 190.0},
        {90.0, 75.0},
        {140.0, 130.0}
    };

    std::vector<Eigen::Vector2d> destination;
    destination.reserve(source.size());

    for(const Eigen::Vector2d& point : source){
        destination.push_back(apply_homography(expected, point));
    }

    Eigen::Matrix3d estimated = compute_homography_dlt(source, destination);
    bool pass = matrix_close(estimated, expected, 1e-6);

    std::cout << name << ": " << (pass ? "PASS" : "FAIL") << "\n";
    return pass;
}

static bool test_ransac(){
    Eigen::Matrix3d expected = Eigen::Matrix3d::Identity();
    expected(0, 2) = 40.0;
    expected(1, 2) = 25.0;

    std::vector<Eigen::Vector2d> source;
    std::vector<Eigen::Vector2d> destination;

    for(int y = 0; y < 8; y++){
        for(int x = 0; x < 10; x++){
            Eigen::Vector2d point(20.0 + x * 17.0, 15.0 + y * 19.0);
            source.push_back(point);
            destination.push_back(apply_homography(expected, point));
        }
    }

    std::mt19937 generator(1234);
    std::uniform_real_distribution<double> distribution(0.0, 250.0);

    // inject 40 correspondences that have nothing to do with the real translation
    for(int i = 0; i < 40; i++){
        source.emplace_back(distribution(generator), distribution(generator));
        destination.emplace_back(distribution(generator), distribution(generator));
    }

    RANSAC_RESULT result = ransac_homography(source, destination);
    bool pass = result.num_inliers >= 80 && matrix_close(result.H, expected, 1e-5);

    std::cout << "ransac outliers: " << (pass ? "PASS" : "FAIL") << "\n";
    return pass;
}

int main(){
    bool pass = true;

    Eigen::Matrix3d identity = Eigen::Matrix3d::Identity();
    pass &= test_homography("identity", identity);

    Eigen::Matrix3d translation = Eigen::Matrix3d::Identity();
    translation(0, 2) = 40.0;
    translation(1, 2) = 25.0;
    pass &= test_homography("translation", translation);

    constexpr double PI = 3.14159265358979323846;
    double angle = 18.0 * PI / 180.0;
    Eigen::Matrix3d rotation = Eigen::Matrix3d::Identity();
    rotation(0, 0) = std::cos(angle);
    rotation(0, 1) = -std::sin(angle);
    rotation(1, 0) = std::sin(angle);
    rotation(1, 1) = std::cos(angle);
    pass &= test_homography("rotation", rotation);

    Eigen::Matrix3d scaling = Eigen::Matrix3d::Identity();
    scaling(0, 0) = 1.35;
    scaling(1, 1) = 0.8;
    pass &= test_homography("scaling", scaling);

    Eigen::Matrix3d perspective;
    perspective << 1.02, 0.04, 18.0,
                  -0.03, 0.97, 11.0,
                   0.0004, -0.0002, 1.0;
    pass &= test_homography("perspective", perspective);

    pass &= test_ransac();

    return pass ? 0 : 1;
}
