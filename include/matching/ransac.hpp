#pragma once

#include <vector>

#include <Eigen/Dense>

struct RANSAC_RESULT{
    Eigen::Matrix3d H = Eigen::Matrix3d::Identity();
    std::vector<int> inliers;
    int num_inliers = 0;
};

RANSAC_RESULT ransac_homography(
    const std::vector<Eigen::Vector2d>& src_points,
    const std::vector<Eigen::Vector2d>& dst_points,
    double inlier_threshold = 3.0,
    int max_iterations = 1000,
    double confidence = 0.99
);
