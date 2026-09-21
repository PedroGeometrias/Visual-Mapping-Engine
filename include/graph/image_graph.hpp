#pragma once

#include <cstddef>
#include <vector>

#include <Eigen/Dense>

#include "features/descriptor.hpp"
#include "matching/matcher.hpp"

struct IMAGE_EDGE{
    std::size_t image_a = 0;
    std::size_t image_b = 0;
    Eigen::Matrix3d homography_a_to_b = Eigen::Matrix3d::Identity();
    int inliers = 0;
    double mean_reprojection_error = 0.0;
    std::vector<FEATURE_MATCH> matches;
    std::vector<int> inlier_indices;
};

struct IMAGE_GRAPH{
    std::size_t image_count = 0;
    std::vector<IMAGE_EDGE> edges;
};

IMAGE_GRAPH build_image_graph(
    const std::vector<std::vector<FEATURE_DESCRIPTOR>>& descriptors
);

std::vector<IMAGE_EDGE> maximum_spanning_tree(const IMAGE_GRAPH& graph);

std::size_t choose_reference_image(
    std::size_t image_count,
    const std::vector<IMAGE_EDGE>& tree
);

bool compute_image_transforms(
    std::size_t image_count,
    const std::vector<IMAGE_EDGE>& tree,
    std::size_t reference_image,
    std::vector<Eigen::Matrix3d>& transforms_to_reference
);
