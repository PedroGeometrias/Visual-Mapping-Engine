#include "debug/visualization.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>

// this module is really simple, just used for visualization really

static void copy_image(Image& destination, const Image& source, int offset_x){
    for(int y = 0; y < source.height; y++){
        for(int x = 0; x < source.width; x++){
            for(int c = 0; c < 3; c++){
                uint8_t value = source.channels == 1
                    ? source.at(x, y, 0)
                    : source.at(x, y, c);

                destination.at(x + offset_x, y, c) = value;
            }
        }
    }
}

static void set_pixel(
    Image& image,
    int x,
    int y,
    uint8_t r,
    uint8_t g,
    uint8_t b
){
    if(x < 0 || x >= image.width || y < 0 || y >= image.height){
        return;
    }

    image.at(x, y, 0) = r;
    image.at(x, y, 1) = g;
    image.at(x, y, 2) = b;
}

static void draw_line(
    Image& image,
    int x0,
    int y0,
    int x1,
    int y1,
    uint8_t r,
    uint8_t g,
    uint8_t b
){
    int dx = std::abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;

    while(true){
        set_pixel(image, x0, y0, r, g, b);

        if(x0 == x1 && y0 == y1){
            break;
        }

        int twice_error = 2 * error;

        if(twice_error >= dy){
            error += dy;
            x0 += sx;
        }

        if(twice_error <= dx){
            error += dx;
            y0 += sy;
        }
    }
}

static Image make_match_canvas(const Image& image_a, const Image& image_b){
    Image out;
    out.width = image_a.width + image_b.width;
    out.height = std::max(image_a.height, image_b.height);
    out.channels = 3;
    out.pixels.resize(out.width * out.height * out.channels, 0);

    copy_image(out, image_a, 0);
    copy_image(out, image_b, image_a.width);

    return out;
}

Image draw_feature_matches(
    const Image& image_a,
    const Image& image_b,
    const std::vector<FEATURE_DESCRIPTOR>& features_a,
    const std::vector<FEATURE_DESCRIPTOR>& features_b,
    const std::vector<FEATURE_MATCH>& matches
){
    Image out = make_match_canvas(image_a, image_b);

    for(const FEATURE_MATCH& match : matches){
        if(match.feature_a >= features_a.size() || match.feature_b >= features_b.size()){
            continue;
        }

        const HARRIS_FEATURE& a = features_a[match.feature_a].feature;
        const HARRIS_FEATURE& b = features_b[match.feature_b].feature;

        draw_line(
            out,
            a.x,
            a.y,
            image_a.width + b.x,
            b.y,
            0,
            255,
            0
        );
    }

    return out;
}

Image draw_ransac_matches(
    const Image& image_a,
    const Image& image_b,
    const std::vector<FEATURE_DESCRIPTOR>& features_a,
    const std::vector<FEATURE_DESCRIPTOR>& features_b,
    const std::vector<FEATURE_MATCH>& matches,
    const std::vector<int>& inliers
){
    Image out = make_match_canvas(image_a, image_b);
    std::vector<bool> is_inlier(matches.size(), false);

    for(int index : inliers){
        if(index >= 0 && static_cast<std::size_t>(index) < is_inlier.size()){
            is_inlier[index] = true;
        }
    }

    for(std::size_t i = 0; i < matches.size(); i++){
        const FEATURE_MATCH& match = matches[i];

        if(match.feature_a >= features_a.size() || match.feature_b >= features_b.size()){
            continue;
        }

        const HARRIS_FEATURE& a = features_a[match.feature_a].feature;
        const HARRIS_FEATURE& b = features_b[match.feature_b].feature;

        // green means this match agrees with the homography, red means ransac rejected it
        if(is_inlier[i]){
            draw_line(out, a.x, a.y, image_a.width + b.x, b.y, 0, 255, 0);
        }
        else{
            draw_line(out, a.x, a.y, image_a.width + b.x, b.y, 255, 0, 0);
        }
    }

    return out;
}

bool save_image_graph_dot(
    const std::string& filename,
    const IMAGE_GRAPH& graph,
    const std::vector<IMAGE_EDGE>& tree
){
    std::ofstream out(filename);

    if(!out){
        return false;
    }

    out << "graph panorama {\n";
    out << "    node [shape=box];\n";

    for(std::size_t i = 0; i < graph.image_count; i++){
        out << "    " << i << " [label=\"image " << i << "\"];\n";
    }

    for(const IMAGE_EDGE& edge : graph.edges){
        bool in_tree = false;

        for(const IMAGE_EDGE& tree_edge : tree){
            if(tree_edge.image_a == edge.image_a && tree_edge.image_b == edge.image_b){
                in_tree = true;
                break;
            }
        }

        out << "    " << edge.image_a << " -- " << edge.image_b
            << " [label=\"" << edge.inliers << " inliers, "
            << edge.mean_reprojection_error << " px\"";

        if(in_tree){
            out << ", penwidth=3";
        }

        out << "];\n";
    }

    out << "}\n";
    return true;
}
