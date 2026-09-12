#include "image/image_io.hpp"
#include "image/image_processing.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " <input>\n";
        return 1;
    }

    Image img = load_image(argv[1]);

    // Test 1: identity — resize to the same size, must be byte-exact
    Image ident = resize_bilinear(img, img.width, img.height);
    bool ident_ok = (ident.pixels == img.pixels);
    std::cout << "identity: " << (ident_ok ? "PASS" : "FAIL") << "\n";

    // Test 2: 2x downscale — should produce a visible half-size image
    Image half = resize_bilinear(img, img.width / 2, img.height / 2);
    save_image("output/half.png", half);

    // Test 3: 2x upscale — should produce a visible double-size image
    Image dbl = resize_bilinear(img, img.width * 2, img.height * 2);
    save_image("output/double.png", dbl);

    return 0;
}
