#include "features/gradients.hpp"

#include <cmath>

IMAGE_GRADIENTS sobel_gradients(const Image &grayscale){
    IMAGE_GRADIENTS temp_gradients;

    temp_gradients.width = grayscale.width;
    temp_gradients.height = grayscale.height;

    // Allocate vectors and initialize to zero because borders are skipped.
    temp_gradients.x.resize(grayscale.width * grayscale.height, 0.0f);
    temp_gradients.y.resize(grayscale.width * grayscale.height, 0.0f);

    // these the standard kernels for sobel algorith
    int sobel_x[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };
    int sobel_y[3][3] = {
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1}
    };

    // looping through the image
    for(int y = 1; y < grayscale.height - 1; ++y){
        for(int x = 1; x < grayscale.width - 1; ++x){
            // I could abstract all of this using nested loops, and this formula:
            // image_x = x + (i - 1)
            // image_y = y + (j - 1)
            // since the kernel is three by three the limit would be i < 3, and j < 3.
            // but I'm just writing it here because it's easy to visualize
            float top_left     = grayscale.at(x - 1, y - 1, 0);
            float top          = grayscale.at(x,     y - 1, 0);
            float top_right    = grayscale.at(x + 1, y - 1, 0);

            float left         = grayscale.at(x - 1, y, 0);
            float center       = grayscale.at(x,     y, 0);
            float right        = grayscale.at(x + 1, y, 0);

            float bottom_left  = grayscale.at(x - 1, y + 1, 0);
            float bottom       = grayscale.at(x,     y + 1, 0);
            float bottom_right = grayscale.at(x + 1, y + 1, 0);

            float gx =
                top_left     * sobel_x[0][0] +
                top          * sobel_x[0][1] +
                top_right    * sobel_x[0][2] +

                left         * sobel_x[1][0] +
                center       * sobel_x[1][1] +
                right        * sobel_x[1][2] +

                bottom_left  * sobel_x[2][0] +
                bottom       * sobel_x[2][1] +
                bottom_right * sobel_x[2][2];

            float gy =
                top_left     * sobel_y[0][0] +
                top          * sobel_y[0][1] +
                top_right    * sobel_y[0][2] +

                left         * sobel_y[1][0] +
                center       * sobel_y[1][1] +
                right        * sobel_y[1][2] +

                bottom_left  * sobel_y[2][0] +
                bottom       * sobel_y[2][1] +
                bottom_right * sobel_y[2][2];

            int index = y * grayscale.width + x;

            temp_gradients.x[index] = gx;
            temp_gradients.y[index] = gy;
        }
    }

    return temp_gradients;
}


// this was my initial idea, instead of using sobel I could do this instead, turns out that there is a names
// for it, the algorithm is called forward_difference, the idea is to just loop through the image, and get
// the difference one by one, but there is no smothing
IMAGE_GRADIENTS getting_difference(const Image& grayscale){
    IMAGE_GRADIENTS gradients;

    gradients.width = grayscale.width;
    gradients.height = grayscale.height;

    gradients.x.resize(grayscale.width * grayscale.height, 0.0f);
    gradients.y.resize(grayscale.width * grayscale.height, 0.0f);

    for(int y = 0; y < grayscale.height - 1; ++y){
        for(int x = 0; x < grayscale.width - 1; ++x){

            float current = grayscale.at(x, y, 0);
            float right   = grayscale.at(x + 1, y, 0);
            float bottom  = grayscale.at(x, y + 1, 0);

            float gx = right - current;
            float gy = bottom - current;

            int index = y * grayscale.width + x;

            gradients.x[index] = gx;
            gradients.y[index] = gy;
        }
    }

    return gradients;
}

// this is another one, it's called central difference, it's kinda similar to forward difference
// IMAGE_GRADIENTS central_gradients(const Image& grayscale)
IMAGE_GRADIENTS central_gradients(const Image& grayscale){
    IMAGE_GRADIENTS gradients;

    gradients.width = grayscale.width;
    gradients.height = grayscale.height;

    gradients.x.resize(grayscale.width * grayscale.height, 0.0f);
    gradients.y.resize(grayscale.width * grayscale.height, 0.0f);

    for(int y = 1; y < grayscale.height - 1; ++y){
        for(int x = 1; x < grayscale.width - 1; ++x){

            float left   = grayscale.at(x - 1, y, 0);
            float right  = grayscale.at(x + 1, y, 0);

            float top    = grayscale.at(x, y - 1, 0);
            float bottom = grayscale.at(x, y + 1, 0);

            float gx = (right - left) * 0.5f;
            float gy = (bottom - top) * 0.5f;

            int index = y * grayscale.width + x;

            gradients.x[index] = gx;
            gradients.y[index] = gy;
        }
    }

    return gradients;
}

static int gradient_index(const IMAGE_GRADIENTS& gradients, int x, int y){
    return y * gradients.width + x;
}

float gradient_magnitude_at(const IMAGE_GRADIENTS& gradients, int x, int y){
    int index = gradient_index(gradients, x, y);
    float gx = gradients.x[index];
    float gy = gradients.y[index];

    return std::sqrt(gx * gx + gy * gy);
}

float gradient_orientation_at(const IMAGE_GRADIENTS& gradients, int x, int y){
    int index = gradient_index(gradients, x, y);

    return std::atan2(gradients.y[index], gradients.x[index]);
}
