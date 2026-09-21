#include "matching/matcher.hpp"

#include <cmath>
#include <limits>

// euclidian distance between two 128-d descriptor vectors
float descriptor_distance(
    const FEATURE_DESCRIPTOR& a,
    const FEATURE_DESCRIPTOR& b
){
    
    float sum = 0.0f;
    for(std::size_t i = 0; i < a.values.size(); i++){
        float difference = a.values[i] - b.values[i];
        sum += difference * difference;
    }

    return std::sqrt(sum);
}

// this function finds the est and second best matches for one descriptor inside a candidate list, 
// then applies two filters
static bool nearest_feature(
    const FEATURE_DESCRIPTOR& feature,
    const std::vector<FEATURE_DESCRIPTOR>& candidates,
    std::size_t& best_index,
    float& best_distance
){
    // this needs tuning if I find mistakes so [GREP_THIS_LATER]
    constexpr float RATIO_THRESHOLD = 0.8f;
    // this is the threshold, I don't really think there is perfect number here, so I really just gotta
    // do the try and error route [GREP_THIS_LATER] if I find to many mistakes I can just adjust this dude
    constexpr float DISTANCE_THRESHOLD = 0.8f;

    // the maximum distance for a match
    if(candidates.size() < 2){
        return false;
    }

    // second and best
    float second_distance = std::numeric_limits<float>::max();
    best_distance = std::numeric_limits<float>::max();
    best_index = 0;

    for(std::size_t i = 0; i < candidates.size(); i++){
        float distance = descriptor_distance(feature, candidates[i]);

        if(distance < best_distance){
            second_distance = best_distance;
            best_distance = distance;
            best_index = i;
        }
        else if(distance < second_distance){
            second_distance = distance;
        }
    }

    // if the best distance is still far from the image, it's still discarded
    if(best_distance > DISTANCE_THRESHOLD){
        return false;
    }

    // this is called lowes ratio test, if the best match is much closer than the second-best, the match
    // distinctive, if the best and second bestare similar, the match is ambiguos and likely wrong
    if(second_distance <= 1e-6f){
        return false;
    }
    return best_distance / second_distance < RATIO_THRESHOLD;
}

// main matching function, it takes descriptors from image a and b, andreturns matches
std::vector<FEATURE_MATCH> match_features(
    const std::vector<FEATURE_DESCRIPTOR>& features_a,
    const std::vector<FEATURE_DESCRIPTOR>& features_b
){
    // output guy
    std::vector<FEATURE_MATCH> matches;

    // looping over image a descriptors
    for(std::size_t i = 0; i < features_a.size(); i++){
        std::size_t best_b = 0;
        float distance_a_to_b = 0.0f;

        // forward match -> find best B for A
        if(!nearest_feature(features_a[i], features_b, best_b, distance_a_to_b)){
            continue;
        }

        std::size_t best_a = 0;
        float distance_b_to_a = 0.0f;

        // reverse match -> find best A for that B
        if(!nearest_feature(features_b[best_b], features_a, best_a, distance_b_to_a)){
            continue;
        }

        // if A is different then i, then the two fatures are no each other's best match, probably a 
        // false positive
        if(best_a != i){
            continue;
        }

        // we push into the output container, the output guy
        matches.push_back({i, best_b, distance_a_to_b});
    }

    return matches;
}
