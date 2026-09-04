#pragma once

#include "image.hpp"
#include <string>

Image load_image(const std::string& filename);
bool save_image(const std::string& filename, const Image& img);
