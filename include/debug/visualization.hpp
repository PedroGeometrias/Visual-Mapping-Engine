#pragma once

#include <string>
#include <vector>

#include "features/descriptor.hpp"
#include "image/image.hpp"
#include "matching/matcher.hpp"
#include "graph/image_graph.hpp"

Image draw_feature_matches(
    const Image& image_a,
    const Image& image_b,
    const std::vector<FEATURE_DESCRIPTOR>& features_a,
    const std::vector<FEATURE_DESCRIPTOR>& features_b,
    const std::vector<FEATURE_MATCH>& matches
);

Image draw_ransac_matches(
    const Image& image_a,
    const Image& image_b,
    const std::vector<FEATURE_DESCRIPTOR>& features_a,
    const std::vector<FEATURE_DESCRIPTOR>& features_b,
    const std::vector<FEATURE_MATCH>& matches,
    const std::vector<int>& inliers
);


bool save_image_graph_dot(
    const std::string& filename,
    const IMAGE_GRAPH& graph,
    const std::vector<IMAGE_EDGE>& tree
);
