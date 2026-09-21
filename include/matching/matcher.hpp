#pragma once

#include <cstddef>
#include <vector>

#include "features/descriptor.hpp"

struct FEATURE_MATCH{
    std::size_t feature_a = 0;
    std::size_t feature_b = 0;
    float distance = 0.0f;
};

float descriptor_distance(
    const FEATURE_DESCRIPTOR& a,
    const FEATURE_DESCRIPTOR& b
);

std::vector<FEATURE_MATCH> match_features(
    const std::vector<FEATURE_DESCRIPTOR>& features_a,
    const std::vector<FEATURE_DESCRIPTOR>& features_b
);
