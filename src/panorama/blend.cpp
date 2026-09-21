#include "panorama/blend.hpp"

#include <algorithm>

// simple feather blending, pixels near the edge of an image get less authority inside an overlap
// pixels deeper inside the image quickly reach weight 1 so we don't darken areas that only one image covers
double feather_weight(const Image& image, double x, double y){
    constexpr double FEATHER_DISTANCE = 32.0;

    double left = x + 1.0;
    double right = image.width - x;
    double top = y + 1.0;
    double bottom = image.height - y;

    double distance_to_edge = std::min({left, right, top, bottom});
    return std::clamp(distance_to_edge / FEATHER_DISTANCE, 1.0 / FEATHER_DISTANCE, 1.0);
}
