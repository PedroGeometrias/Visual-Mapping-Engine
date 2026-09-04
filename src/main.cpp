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
    Image gray = to_grayscale(img);

    save_image(argv[2], gray);

    return 0;
}
