#include "image/image_processing.hpp"
#include <cmath>

Image to_grayscale(const Image &src){
    Image gray_scaled;
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t gray = 0;
    gray_scaled.width = src.width;
    gray_scaled.height = src.height;
    gray_scaled.channels = 1;
    gray_scaled.pixels.resize(src.width * src.height);
    for(int y = 0; y < src.height; ++y){
        for(int x = 0; x <src.width; ++x){
            r = src.at(x, y, 0);
            g = src.at(x, y, 1);
            b = src.at(x, y, 2);
            
            gray = (0.299 * r) + (0.587 * g) + (0.114 * b);
            gray_scaled.at(x, y, 0) = gray;
        }
    }
    return gray_scaled;
}

// bilinear sampling at continuous coordinates (x, y)
// returns the interpolated value for the requested channel
uint8_t bilinear_sample(const Image& img, double x, double y, int channel){
    // clamp coordinates to image boundaries
    if (x < 0.0) x = 0.0;
    if (y < 0.0) y = 0.0;
    if (x > img.width - 1.0) x = img.width - 1.0;
    if (y > img.height - 1.0) y = img.height - 1.0;

    // static_cast operator will convert the floor(coord) into integers
    int x0 = static_cast<int>(std::floor(x));
    int y0 = static_cast<int>(std::floor(y));
    int x1 = std::min(x0 + 1, img.width - 1);
    int y1 = std::min(y0 + 1, img.height - 1);

    // fractional parts
    double dx = x - x0;
    double dy = y - y0;

    // weights for the four corners
    double w00 = (1.0 - dx) * (1.0 - dy);
    double w01 = dx * (1.0 - dy);
    double w10 = (1.0 - dx) * dy;
    double w11 = dx * dy;

    // interpolate the requested channel
    double val =
        w00 * img.at(x0, y0, channel) +
        w01 * img.at(x1, y0, channel) +
        w10 * img.at(x0, y1, channel) +
        w11 * img.at(x1, y1, channel);

    // convert to uint8_t and round to the nearest integer
    return static_cast<uint8_t>(val + 0.5);
}
