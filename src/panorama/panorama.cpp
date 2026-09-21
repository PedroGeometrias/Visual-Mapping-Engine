#include "panorama/panorama.hpp"

#include "geometry/homography.hpp"
#include "image/image_processing.hpp"
#include "panorama/blend.hpp"
#include "panorama/warp.hpp"

#include <algorithm>
#include <cmath>
#include <vector>


static Image crop_to_coverage(
    const Image& image,
    const std::vector<float>& weights
){
    int min_x = image.width;
    int min_y = image.height;
    int max_x = -1;
    int max_y = -1;

    for(int y = 0; y < image.height; y++){
        for(int x = 0; x < image.width; x++){
            if(weights[y * image.width + x] <= 0.0f){
                continue;
            }

            min_x = std::min(min_x, x);
            min_y = std::min(min_y, y);
            max_x = std::max(max_x, x);
            max_y = std::max(max_y, y);
        }
    }

    if(max_x < min_x || max_y < min_y){
        return {};
    }

    if(min_x == 0 && min_y == 0 &&
       max_x == image.width - 1 && max_y == image.height - 1){
        return image;
    }

    Image cropped;
    cropped.width = max_x - min_x + 1;
    cropped.height = max_y - min_y + 1;
    cropped.channels = image.channels;
    cropped.pixels.resize(cropped.width * cropped.height * cropped.channels);

    // trim rows and columns that no warped image ever touched, but keep real black pixels inside the panorama
    for(int y = 0; y < cropped.height; y++){
        for(int x = 0; x < cropped.width; x++){
            for(int c = 0; c < cropped.channels; c++){
                cropped.at(x, y, c) = image.at(x + min_x, y + min_y, c);
            }
        }
    }

    return cropped;
}

Image reconstruct_panorama(
    const std::vector<Image>& images,
    const std::vector<Eigen::Matrix3d>& transforms_to_reference
){
    Image panorama;

    if(images.empty() || images.size() != transforms_to_reference.size()){
        return panorama;
    }

    WARP_BOUNDS world_bounds = panorama_bounds(images, transforms_to_reference);

    if(world_bounds.width() <= 0 || world_bounds.height() <= 0){
        return panorama;
    }

    Eigen::Matrix3d translation = bounds_translation(world_bounds);

    panorama.width = world_bounds.width();
    panorama.height = world_bounds.height();
    panorama.channels = 3;
    panorama.pixels.resize(panorama.width * panorama.height * panorama.channels, 0);

    // keep floating point sums until every image has contributed, otherwise repeated uint8 rounding creates seams
    std::vector<float> color_sum(
        panorama.width * panorama.height * panorama.channels,
        0.0f
    );
    std::vector<float> weight_sum(panorama.width * panorama.height, 0.0f);

    for(std::size_t image_index = 0; image_index < images.size(); image_index++){
        const Image& image = images[image_index];
        Eigen::Matrix3d image_to_panorama = translation * transforms_to_reference[image_index];
        Eigen::Matrix3d panorama_to_image = image_to_panorama.inverse();

        // don't scan the entire panorama for every image, only scan the rectangle where this warped image can exist
        WARP_BOUNDS bounds = transformed_image_bounds(image, image_to_panorama);
        int start_x = std::max(0, bounds.min_x);
        int start_y = std::max(0, bounds.min_y);
        int end_x = std::min(panorama.width - 1, bounds.max_x);
        int end_y = std::min(panorama.height - 1, bounds.max_y);

        for(int y = start_y; y <= end_y; y++){
            for(int x = start_x; x <= end_x; x++){
                // inverse mapping asks where this destination pixel came from in the source image
                // doing it forward would leave holes because transformed source pixels rarely land exactly on integer positions
                Eigen::Vector2d source = apply_homography(
                    panorama_to_image,
                    Eigen::Vector2d(x, y)
                );

                if(!std::isfinite(source.x()) || !std::isfinite(source.y())){
                    continue;
                }

                if(source.x() < 0.0 || source.x() > image.width - 1.0 ||
                   source.y() < 0.0 || source.y() > image.height - 1.0){
                    continue;
                }

                float weight = static_cast<float>(feather_weight(image, source.x(), source.y()));
                int pixel_index = y * panorama.width + x;

                for(int c = 0; c < panorama.channels; c++){
                    int source_channel = image.channels == 1 ? 0 : c;
                    float value = bilinear_sample(
                        image,
                        source.x(),
                        source.y(),
                        source_channel
                    );

                    color_sum[pixel_index * panorama.channels + c] += value * weight;
                }

                weight_sum[pixel_index] += weight;
            }
        }
    }

    for(int y = 0; y < panorama.height; y++){
        for(int x = 0; x < panorama.width; x++){
            int pixel_index = y * panorama.width + x;
            float weight = weight_sum[pixel_index];

            if(weight <= 0.0f){
                continue;
            }

            for(int c = 0; c < panorama.channels; c++){
                float value = color_sum[pixel_index * panorama.channels + c] / weight;
                value = std::clamp(value, 0.0f, 255.0f);
                panorama.at(x, y, c) = static_cast<uint8_t>(value + 0.5);
            }
        }
    }

    return crop_to_coverage(panorama, weight_sum);
}
