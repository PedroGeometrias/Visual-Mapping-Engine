#include "panorama/warp.hpp"

#include "geometry/homography.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

WARP_BOUNDS transformed_image_bounds(
    const Image& image,
    const Eigen::Matrix3d& transform
){
    WARP_BOUNDS bounds;

    if(image.width <= 0 || image.height <= 0){
        return bounds;
    }

    const Eigen::Vector2d corners[4] = {
        {0.0, 0.0},
        {static_cast<double>(image.width - 1), 0.0},
        {0.0, static_cast<double>(image.height - 1)},
        {static_cast<double>(image.width - 1), static_cast<double>(image.height - 1)}
    };

    double min_x = std::numeric_limits<double>::infinity();
    double min_y = std::numeric_limits<double>::infinity();
    double max_x = -std::numeric_limits<double>::infinity();
    double max_y = -std::numeric_limits<double>::infinity();

    for(const Eigen::Vector2d& corner : corners){
        Eigen::Vector2d projected = apply_homography(transform, corner);

        if(!std::isfinite(projected.x()) || !std::isfinite(projected.y())){
            continue;
        }

        min_x = std::min(min_x, projected.x());
        min_y = std::min(min_y, projected.y());
        max_x = std::max(max_x, projected.x());
        max_y = std::max(max_y, projected.y());
    }

    if(!std::isfinite(min_x) || !std::isfinite(min_y) ||
       !std::isfinite(max_x) || !std::isfinite(max_y)){
        return bounds;
    }

    bounds.min_x = static_cast<int>(std::floor(min_x));
    bounds.min_y = static_cast<int>(std::floor(min_y));
    bounds.max_x = static_cast<int>(std::ceil(max_x));
    bounds.max_y = static_cast<int>(std::ceil(max_y));

    return bounds;
}

WARP_BOUNDS panorama_bounds(
    const std::vector<Image>& images,
    const std::vector<Eigen::Matrix3d>& transforms
){
    WARP_BOUNDS bounds;

    if(images.empty() || images.size() != transforms.size()){
        return bounds;
    }

    bool has_bounds = false;

    for(std::size_t i = 0; i < images.size(); i++){
        WARP_BOUNDS image_bounds = transformed_image_bounds(images[i], transforms[i]);

        if(image_bounds.width() <= 0 || image_bounds.height() <= 0){
            continue;
        }

        if(!has_bounds){
            bounds = image_bounds;
            has_bounds = true;
            continue;
        }

        bounds.min_x = std::min(bounds.min_x, image_bounds.min_x);
        bounds.min_y = std::min(bounds.min_y, image_bounds.min_y);
        bounds.max_x = std::max(bounds.max_x, image_bounds.max_x);
        bounds.max_y = std::max(bounds.max_y, image_bounds.max_y);
    }

    return bounds;
}

Eigen::Matrix3d bounds_translation(const WARP_BOUNDS& bounds){
    Eigen::Matrix3d translation = Eigen::Matrix3d::Identity();

    // homographies can put valid pixels at negative coordinates, shift the whole world back into image space
    translation(0, 2) = -bounds.min_x;
    translation(1, 2) = -bounds.min_y;

    return translation;
}
