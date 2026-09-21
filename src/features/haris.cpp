/*
 * for more info on harris transformation, you can go look up the wikipedia article, it's really good, and
 * it also provides some cool resources, but this entire algorithm is meant to find corners in images, 
 * a corner being a junction of two edges, and adges basically being a big change of brighteness of pixels,
 * we call these corners features, the algorithm can be distilled by these steps:
 * grayscale the image -> we do this because we really just care about brightness here, since we are detecting
 * changes in that, this makes the algorthm more efficient, sicne we are discarding useless data
 * spacitial derivatives -> here we take the spatial derivative of x and y, here we are gonna use sobel algorithm to approcimate 
 * said derivative
 * struture the tensor response -> in here, we are not creating a matrix per pixel, we are just storing them into 
 * separate entries
*  harris response calculation -> this one is trick, the wiki doesn't explain it clearly enough in my opinion, but first 
*  we "compute" the smallest eigenvalue of the tensor, using an approximation, but to be clear, we don't compute it, because:
*  the eigen values for a 2 by 2 matrix are the roots od the characteric polynomial, but here we really need to know trace and det,
*  so we can just skip the whole square root thing, we reall wanna know how big those two fellas are
*
*/

#include "features/haris.hpp"

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

std::vector<HARRIS_FEATURE> harris_features(const IMAGE_GRADIENTS& gradients){
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
