#include "image/image_processing.hpp"

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
