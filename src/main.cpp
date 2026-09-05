#include "image/image_io.hpp"
#include "image/image_processing.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "usage: " << argv[0]
                  << " <input> <output>\n";
        return 1;
    }

    Image img = load_image(argv[1]);

    Image sampled;
    sampled.width = img.width;
    sampled.height = img.height;
    sampled.channels = img.channels;
    sampled.pixels.resize(
        sampled.width * sampled.height * sampled.channels
    );

    for (int y = 0; y < sampled.height; ++y) {
        for (int x = 0; x < sampled.width; ++x) {
            for (int c = 0; c < sampled.channels; ++c) {
                sampled.at(x, y, c) =
                    bilinear_sample(img, x + 0.5, y + 0.5, c);
            }
        }
    }

    save_image(argv[2], sampled);

    return 0;
}
