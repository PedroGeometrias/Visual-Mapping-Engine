#pragma once

#include <vector>
#include "features/gradients.hpp"
#include "image/image.hpp"

struct HARRIS_FEATURE{
    int x;
    int y;
    float response;
};

std::vector<HARRIS_FEATURE> harris_features(const IMAGE_GRADIENTS& gradients);
Image draw_harris_features(const Image& image, const std::vector<HARRIS_FEATURE>& features);
