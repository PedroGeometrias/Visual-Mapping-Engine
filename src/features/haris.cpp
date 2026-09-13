#include "features/haris.hpp"

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

// I loop through the guy, sum the pixel using that formula times the weight, and then I divide it by 16, 
// 16 being the sum of the kernel
static std::vector<float> gaussian_blur(
    const std::vector<float>& in,
    int width,
    int height
){
    std::vector<float> out(width * height, 0.0f);
    const int gaussian[3][3] = {
        {1, 2, 1},
        {2, 4, 2},
        {1, 2, 1}
    };
    for(int y = 1; y < height - 1; ++y){
        for(int x = 1; x < width - 1; ++x){
            float sum = 0.0f;
            for(int ky = 0; ky < 3; ++ky){
                for(int kx = 0; kx < 3; ++kx){
                    int image_x = x + kx - 1;
                    int image_y = y + ky - 1;
                    int image_index = image_y * width + image_x;
                    sum += in[image_index] * gaussian[ky][kx];
                }
            }
            int index = y * width + x;
            out[index] = sum / 16.0f;
        }
    }
    return out;
}

static std::vector<float> harris_response(
    const IMAGE_GRADIENTS& gradients
){

    // Products — each is a float vector of size w*h
    std::vector<float> Ixx(gradients.width*gradients.height), Iyy(gradients.width*gradients.height), Ixy(gradients.width*gradients.height);
    int w = gradients.width;
    int h = gradients.height;
    for (int i = 0; i < w*h; ++i) {
        Ixx[i] = gradients.x[i] * gradients.x[i];
        Iyy[i] = gradients.y[i] * gradients.y[i];
        Ixy[i] = gradients.x[i] * gradients.y[i];
    }

    // 3. Blur each product (the "window")
    Ixx = gaussian_blur(Ixx, w, h);
    Iyy = gaussian_blur(Iyy, w, h);
    Ixy = gaussian_blur(Ixy, w, h);

    // 4. Harris response per pixel
    std::vector<float> R(w*h, 0.0f);
    const float k = 0.04f;
    for (int i = 0; i < w*h; ++i) {
        float det   = Ixx[i] * Iyy[i] - Ixy[i] * Ixy[i];
        float trace = Ixx[i] + Iyy[i];
        R[i] = det - k * trace * trace;

    }
    return R;
}

static std::vector<HARRIS_FEATURE> non_maximum_suppression(
    const std::vector<float>& response,
    int width,
    int height,
    float threshold,
    int radius
){
    std::vector<HARRIS_FEATURE> features;

    for(int y = radius; y < height - radius; ++y){
        for(int x = radius; x < width - radius; ++x){
            int index = y * width + x;
            float current = response[index];

            if(current <= threshold){
                continue;
            }

            bool is_maximum = true;

            for(int ny = y - radius; ny <= y + radius && is_maximum; ++ny){
                for(int nx = x - radius; nx <= x + radius; ++nx){
                    if(nx == x && ny == y){
                        continue;
                    }

                    int neighbor_index = ny * width + nx;
                    if(response[neighbor_index] > current){
                        is_maximum = false;
                        break;
                    }
                }
            }

            if(is_maximum){
                features.push_back({x, y, current});
            }
        }
    }

    return features;
}

std::vector<HARRIS_FEATURE> harris_features(const Image& grayscale){
    IMAGE_GRADIENTS gradients = sobel_gradients(grayscale);
    std::vector<float> response = harris_response(gradients);

    float max_response = 0.0f;
    for(float value : response){
        if(value > max_response){
            max_response = value;
        }
    }

    float threshold = max_response * 0.01f;

    return non_maximum_suppression(
        response,
        gradients.width,
        gradients.height,
        threshold,
        2
    );
}

Image draw_harris_features(
    const Image& image,
    const std::vector<HARRIS_FEATURE>& features
){
    Image out = image;

    for(const HARRIS_FEATURE& feature : features){
        for(int dy = -2; dy <= 2; ++dy){
            for(int dx = -2; dx <= 2; ++dx){
                int x = feature.x + dx;
                int y = feature.y + dy;

                if(x < 0 || x >= out.width || y < 0 || y >= out.height){
                    continue;
                }

                if(out.channels >= 3){
                    out.at(x, y, 0) = 255;
                    out.at(x, y, 1) = 0;
                    out.at(x, y, 2) = 0;
                }
            }
        }
    }

    return out;
}
