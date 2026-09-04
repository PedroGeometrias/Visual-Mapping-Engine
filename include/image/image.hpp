#pragma once

#include <vector>
#include <stdint.h>

// In C++, structs can contain member functions, not just data.
struct Image {
    int width = 0;
    int height = 0;

    // 1 for grayscale and 3 for RGB
    int channels = 3;

    // Row major, size = width * height * channels

    // std is a namespace used to group names and avoid collisions.
    // :: is the scope resolution operator.
    // std::vector means "vector from the std namespace".
    //
    // vector is a class template: its implementation can work with
    // different element types.
    // Here we instantiate it with uint8_t, so pixels is a vector of uint8_t.
    std::vector<uint8_t> pixels;

    // Accessors, mutable and immutable.

    // & means this function returns a reference.
    // More specifically, it returns a reference/alias to the actual
    // uint8_t element stored at this index inside pixels.
    uint8_t& at(int x, int y, int c) {
        return pixels[(y * width + x) * channels + c];
    }

    const uint8_t& at(int x, int y, int c) const {
        return pixels[(y * width + x) * channels + c];
    }
};
