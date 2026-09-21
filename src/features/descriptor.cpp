#include "features/descriptor.hpp"
#include <cmath>

// Harris tells us where the interesting point is.
// this computes the dominat gradient orientation around a harris corner, basically meaning that:
// before we descrbe what they corner looks like, we need to know which way it's pointing, so the descriptor
// becomes rotation-invariant
float dominant_orientation(
    const IMAGE_GRADIENTS& gradients,
    const HARRIS_FEATURE& feature
){
    // 36 means that we can store into our histogram 36 variations of 10 degrees each
    constexpr int ORIENTATION_BINS = 36;
    // 17 * 17 window around the corner
    constexpr int RADIUS = 8;
    // we need pi for angle calculations
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    // around 0.1745 rad
    constexpr float BIN_SIZE = TWO_PI / ORIENTATION_BINS;

    // our histogram to store the orientations
    std::array<float, ORIENTATION_BINS> histogram{};

    // loop over each patch
    for(int y = feature.y - RADIUS; y <= feature.y + RADIUS; y++){
        for(int x = feature.x - RADIUS; x <= feature.x + RADIUS; x++){
            // bound checking
            if(x < 0 || x >= gradients.width ||
               y < 0 || y >= gradients.height){
                continue;
            }

            // angle is the direction of the local gradient
            float angle = gradient_orientation_at(gradients, x, y);
            // how strong that gradient is
            float magnitude = gradient_magnitude_at(gradients, x, y);

            // wraps to [0, 2pi]
            if(angle < 0.0f){
                angle += TWO_PI;
            }
            // flor bin
            int bin = static_cast<int>(angle / BIN_SIZE);
            // safeguarding the angle
            if(bin >= ORIENTATION_BINS){
                bin = 0;
            }
            // storing that magnitude, that's why we floor it, we are using that as an idex
            histogram[bin] += magnitude;
        }
    }
    // FINDING THE PEAK // 
    int dominant_bin = 0;
    // here we just loop, grabbing each peak and saving it into the variable
    for(int i = 1; i < ORIENTATION_BINS; i++){
        if(histogram[i] > histogram[dominant_bin]){
            dominant_bin = i;
        }
    }
    // converting into an angle again
    return dominant_bin * BIN_SIZE;
}

// this builds a SIFT like descriptor for on harris corner, the overall idea is:
// Take a 16×16 patch centered on the feature, rotate it so the feature’s dominant 
// gradient direction becomes the reference direction, split it into 4×4 cells, 
// and for each cell accumulate gradient magnitudes into 8 orientation bins.
FEATURE_DESCRIPTOR describe_feature(
    const IMAGE_GRADIENTS& gradients, 
    const HARRIS_FEATURE& feature
){
    constexpr int CELLS = 4;
    constexpr int CELL_SIZE = 4;
    // 16
    constexpr int WINDOW = CELLS * CELL_SIZE;
    constexpr int ORIENTATION_BINS = 8;
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr float BIN_SIZE = TWO_PI / ORIENTATION_BINS;

    const float theta = dominant_orientation(gradients, feature);
    const float cos_t = std::cos(theta);
    const float sin_t = std::sin(theta);
    // assumes .values is zero-initialized
    FEATURE_DESCRIPTOR descriptor{};
    descriptor.feature = feature;
    descriptor.orientation = theta;

    for (int cy = 0; cy < CELLS; ++cy) {
        for (int cx = 0; cx < CELLS; ++cx) {
            for (int py = 0; py < CELL_SIZE; ++py) {
                for (int px = 0; px < CELL_SIZE; ++px) {

                    //  canonical offset from feature center
                    float dx = (cx * CELL_SIZE + px) - (WINDOW / 2) + 0.5f;
                    float dy = (cy * CELL_SIZE + py) - (WINDOW / 2) + 0.5f;

                    // rotate sampling offset by theta
                    float rx = dx * cos_t - dy * sin_t;
                    float ry = dx * sin_t + dy * cos_t;

                    int sx = static_cast<int>(std::round(feature.x + rx));
                    int sy = static_cast<int>(std::round(feature.y + ry));

                    if (sx < 0 || sx >= gradients.width ||
                        sy < 0 || sy >= gradients.height) continue;

                    // gradient angle relative to dominant orientation
                    float angle = gradient_orientation_at(gradients, sx, sy);
                    float mag   = gradient_magnitude_at  (gradients, sx, sy);

                    angle -= theta;
                    while (angle < 0.0f)      angle += TWO_PI;
                    while (angle >= TWO_PI)   angle -= TWO_PI;

                    int bin = static_cast<int>(angle / BIN_SIZE);
                    if (bin >= ORIENTATION_BINS) bin = ORIENTATION_BINS - 1;

                    //  accumulate into the right cell's histogram
                    int cell_index = cy * CELLS + cx;
                    descriptor.values[cell_index * ORIENTATION_BINS + bin] += mag;
                }
            }
        }
    }

    // L2 normalize the 128-vector
    float norm = 0.0f;
    for (float v : descriptor.values) norm += v * v;
    norm = std::sqrt(norm);
    if (norm > 1e-6f) {
        for (float& v : descriptor.values) v /= norm;
    }

    return descriptor;
}


// this just calls describe_feature for a bunch of features, and pushes them into a vector, simple stuff
std::vector<FEATURE_DESCRIPTOR> describe_features(
    const IMAGE_GRADIENTS& gradients,
    const std::vector<HARRIS_FEATURE>& features
){
    // the rotated 16x16 window can reach about 11 pixels away from the center
    constexpr int BORDER_MARGIN = 12;

    std::vector<FEATURE_DESCRIPTOR> descriptors;
    descriptors.reserve(features.size());

    for(const HARRIS_FEATURE& feature : features){
        if(feature.x < BORDER_MARGIN || feature.x >= gradients.width - BORDER_MARGIN ||
           feature.y < BORDER_MARGIN || feature.y >= gradients.height - BORDER_MARGIN){
            continue;
        }

        descriptors.push_back(describe_feature(gradients, feature));
    }

    return descriptors;
}
