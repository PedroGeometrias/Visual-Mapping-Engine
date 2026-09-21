#pragma once

#include "image.hpp"
#include <cstddef>
#include <cstdint>
#include <string>

Image load_image(const std::string& filename);
Image load_image_memory(const uint8_t *data, std::size_t size);
bool save_image(const std::string& filename, const Image& img);
