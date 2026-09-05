#pragma once

#include "image.hpp"

Image to_grayscale(const Image & src);
uint8_t bilinear_sample(const Image& img, double x, double y, int channel);
