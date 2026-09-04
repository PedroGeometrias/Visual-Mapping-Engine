#include "image/image_io.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

Image load_image(const std::string& filename) {
    Image img;
    int w, h, comp;
    // force rgb
    unsigned char* data = stbi_load(filename.c_str(), &w, &h, &comp, 3);
    if (!data) {
        // handle error
        return img;
    }
    img.width = w;
    img.height = h;
    img.channels = 3;
    img.pixels.assign(data, data + w * h * 3);
    stbi_image_free(data);
    return img;
}

bool save_image(const std::string& filename, const Image& img) {
    if (img.channels == 1) {
        return stbi_write_png(filename.c_str(), img.width, img.height, 1, img.pixels.data(), img.width) != 0;
    } else if (img.channels == 3) {
        return stbi_write_png(filename.c_str(), img.width, img.height, 3, img.pixels.data(), img.width * 3) != 0;
    }
    return false;
}
