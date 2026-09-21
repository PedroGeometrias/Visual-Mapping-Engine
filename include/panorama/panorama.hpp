#pragma once

#include <vector>

#include <Eigen/Dense>

#include "image/image.hpp"

Image reconstruct_panorama(
    const std::vector<Image>& images,
    const std::vector<Eigen::Matrix3d>& transforms_to_reference
);
