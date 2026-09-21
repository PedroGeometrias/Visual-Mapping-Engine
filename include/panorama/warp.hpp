#pragma once

#include <vector>

#include <Eigen/Dense>

#include "image/image.hpp"

struct WARP_BOUNDS{
    int min_x = 0;
    int min_y = 0;
    int max_x = -1;
    int max_y = -1;

    int width() const{
        return max_x >= min_x ? max_x - min_x + 1 : 0;
    }

    int height() const{
        return max_y >= min_y ? max_y - min_y + 1 : 0;
    }
};

WARP_BOUNDS transformed_image_bounds(
    const Image& image,
    const Eigen::Matrix3d& transform
);

WARP_BOUNDS panorama_bounds(
    const std::vector<Image>& images,
    const std::vector<Eigen::Matrix3d>& transforms
);

Eigen::Matrix3d bounds_translation(const WARP_BOUNDS& bounds);
