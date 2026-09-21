#include "image/image_io.hpp"
#include "panorama/pipeline.hpp"

#include <emscripten/emscripten.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

static std::vector<Image> input_images;
static Image panorama_image;
static std::string last_error_message;

extern "C"{

EMSCRIPTEN_KEEPALIVE
void vme_reset(){
    input_images.clear();
    panorama_image = {};
    last_error_message.clear();
}

EMSCRIPTEN_KEEPALIVE
int vme_add_image(const uint8_t *data, int size){
    if(!data || size <= 0){
        last_error_message = "The browser provided an empty image.";
        return 0;
    }

    Image image = load_image_memory(data, static_cast<std::size_t>(size));

    if(image.width <= 0 || image.height <= 0){
        last_error_message = "Could not decode one of the selected images.";
        return 0;
    }

    input_images.push_back(std::move(image));
    last_error_message.clear();
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int vme_build_panorama(){
    PANORAMA_RESULT result = build_panorama(input_images);

    if(!result.error.empty()){
        panorama_image = {};
        last_error_message = std::move(result.error);
        return 0;
    }

    panorama_image = std::move(result.image);
    last_error_message.clear();
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int vme_panorama_width(){
    return panorama_image.width;
}

EMSCRIPTEN_KEEPALIVE
int vme_panorama_height(){
    return panorama_image.height;
}

EMSCRIPTEN_KEEPALIVE
int vme_panorama_size(){
    return static_cast<int>(panorama_image.pixels.size());
}

EMSCRIPTEN_KEEPALIVE
const uint8_t *vme_panorama_pixels(){
    if(panorama_image.pixels.empty()){
        return nullptr;
    }

    return panorama_image.pixels.data();
}

EMSCRIPTEN_KEEPALIVE
const char *vme_last_error(){
    return last_error_message.c_str();
}

}
