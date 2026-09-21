#pragma once

#include <vector>

#include <Eigen/Dense>

Eigen::Matrix3d compute_homography_dlt(
    const std::vector<Eigen::Vector2d>& src_points,
    const std::vector<Eigen::Vector2d>& dst_points
);

Eigen::Vector2d apply_homography(
    const Eigen::Matrix3d& H,
    const Eigen::Vector2d& point
);

double reprojection_error(
    const Eigen::Matrix3d& H,
    const Eigen::Vector2d& source,
    const Eigen::Vector2d& destination
);

bool are_collinear(const std::vector<Eigen::Vector2d>& points);
