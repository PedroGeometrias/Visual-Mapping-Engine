#include <vector>
#include "image/image.hpp"

struct IMAGE_GRADIENTS{
    int width, height;
    std::vector<float> x;
    std::vector<float> y;
};

struct HARRIS_FEATURE{
    int x;
    int y;
    float response;
};

IMAGE_GRADIENTS sobel_gradients(const Image& grayscale);
IMAGE_GRADIENTS getting_difference(const Image& grayscale);
IMAGE_GRADIENTS central_gradient(const Image& grayscale);
std::vector<HARRIS_FEATURE> harris_features(const Image& grayscale);
Image draw_harris_features(const Image& image, const std::vector<HARRIS_FEATURE>& features);
