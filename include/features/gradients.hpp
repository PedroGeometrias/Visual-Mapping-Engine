#pragma once

#include <vector>
#include "image/image.hpp"

struct IMAGE_GRADIENTS{
    int width, height;
    std::vector<float> x;
    std::vector<float> y;
};

IMAGE_GRADIENTS sobel_gradients(const Image& grayscale);
IMAGE_GRADIENTS getting_difference(const Image& grayscale);
IMAGE_GRADIENTS central_gradients(const Image& grayscale);
float gradient_magnitude_at(const IMAGE_GRADIENTS& gradients, int x, int y);
float gradient_orientation_at(const IMAGE_GRADIENTS& gradients, int x, int y);
