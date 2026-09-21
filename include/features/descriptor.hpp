#pragma once

#include <array>
#include <vector>

#include "features/gradients.hpp"
#include "features/haris.hpp"

struct FEATURE_DESCRIPTOR{
    HARRIS_FEATURE feature;
    float orientation = 0.0f;
    std::array<float, 128> values{};
};

float dominant_orientation(
    const IMAGE_GRADIENTS& gradients,
    const HARRIS_FEATURE& feature
);

FEATURE_DESCRIPTOR describe_feature(
    const IMAGE_GRADIENTS& gradients,
    const HARRIS_FEATURE& feature
);

std::vector<FEATURE_DESCRIPTOR> describe_features(
    const IMAGE_GRADIENTS& gradients,
    const std::vector<HARRIS_FEATURE>& features
);
