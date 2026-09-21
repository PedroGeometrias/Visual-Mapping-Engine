#pragma once

#include <string>
#include <vector>

#include "image/image.hpp"

struct PANORAMA_RESULT{
    Image image;
    std::string error;
};

PANORAMA_RESULT build_panorama(const std::vector<Image>& images);
