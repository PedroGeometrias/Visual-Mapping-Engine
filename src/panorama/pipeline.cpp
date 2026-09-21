#include "panorama/pipeline.hpp"

#include "features/descriptor.hpp"
#include "features/gradients.hpp"
#include "features/haris.hpp"
#include "graph/image_graph.hpp"
#include "image/image_processing.hpp"
#include "panorama/panorama.hpp"

#include <utility>
#include <vector>

struct IMAGE_ANALYSIS{
    IMAGE_GRADIENTS gradients;
    std::vector<HARRIS_FEATURE> features;
    std::vector<FEATURE_DESCRIPTOR> descriptors;
};

static IMAGE_ANALYSIS analyze_image(const Image& image){
    IMAGE_ANALYSIS analysis;
    Image grayscale = to_grayscale(image);
    analysis.gradients = sobel_gradients(grayscale);
    analysis.features = harris_features(analysis.gradients);
    analysis.descriptors = describe_features(analysis.gradients, analysis.features);
    return analysis;
}

PANORAMA_RESULT build_panorama(const std::vector<Image>& images){
    PANORAMA_RESULT result;

    if(images.empty()){
        result.error = "No images were provided.";
        return result;
    }

    if(images.size() == 1){
        result.image = images[0];
        return result;
    }

    std::vector<std::vector<FEATURE_DESCRIPTOR>> descriptor_sets;
    descriptor_sets.reserve(images.size());

    for(const Image& image : images){
        IMAGE_ANALYSIS analysis = analyze_image(image);
        descriptor_sets.push_back(std::move(analysis.descriptors));
    }

    IMAGE_GRAPH graph = build_image_graph(descriptor_sets);
    std::vector<IMAGE_EDGE> tree = maximum_spanning_tree(graph);

    if(tree.size() + 1 != images.size()){
        result.error = "The images could not be connected into a panorama.";
        return result;
    }

    std::size_t reference_image = choose_reference_image(images.size(), tree);
    std::vector<Eigen::Matrix3d> transforms_to_reference;

    if(!compute_image_transforms(
        images.size(),
        tree,
        reference_image,
        transforms_to_reference
    )){
        result.error = "Failed to compose the image transforms.";
        return result;
    }

    result.image = reconstruct_panorama(images, transforms_to_reference);

    if(result.image.width <= 0 || result.image.height <= 0){
        result.image = {};
        result.error = "Failed to reconstruct the panorama.";
    }

    return result;
}
