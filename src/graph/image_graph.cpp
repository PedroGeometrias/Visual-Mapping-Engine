#include "graph/image_graph.hpp"

#include "geometry/homography.hpp"
#include "matching/ransac.hpp"

#include <algorithm>
#include <queue>
#include <utility>

class DISJOINT_SET{
public:
    explicit DISJOINT_SET(std::size_t count)
        : parent(count), rank(count, 0)
    {
        for(std::size_t i = 0; i < count; i++){
            parent[i] = i;
        }
    }

    std::size_t find(std::size_t value){
        if(parent[value] != value){
            parent[value] = find(parent[value]);
        }

        return parent[value];
    }

    bool join(std::size_t a, std::size_t b){
        a = find(a);
        b = find(b);

        if(a == b){
            return false;
        }

        if(rank[a] < rank[b]){
            std::swap(a, b);
        }

        parent[b] = a;

        if(rank[a] == rank[b]){
            rank[a]++;
        }

        return true;
    }

private:
    std::vector<std::size_t> parent;
    std::vector<int> rank;
};

IMAGE_GRAPH build_image_graph(
    const std::vector<std::vector<FEATURE_DESCRIPTOR>>& descriptors
){
    IMAGE_GRAPH graph;
    graph.image_count = descriptors.size();

    // we don't assume the input order means anything, try every image pair and let geometry tell us who overlaps
    for(std::size_t a = 0; a < descriptors.size(); a++){
        for(std::size_t b = a + 1; b < descriptors.size(); b++){
            std::vector<FEATURE_MATCH> matches = match_features(descriptors[a], descriptors[b]);

            if(matches.size() < 4){
                continue;
            }

            std::vector<Eigen::Vector2d> source_points;
            std::vector<Eigen::Vector2d> destination_points;
            source_points.reserve(matches.size());
            destination_points.reserve(matches.size());

            for(const FEATURE_MATCH& match : matches){
                const HARRIS_FEATURE& source = descriptors[a][match.feature_a].feature;
                const HARRIS_FEATURE& destination = descriptors[b][match.feature_b].feature;

                source_points.emplace_back(source.x, source.y);
                destination_points.emplace_back(destination.x, destination.y);
            }

            RANSAC_RESULT ransac = ransac_homography(source_points, destination_points);

            if(ransac.num_inliers < 4){
                continue;
            }

            double mean_error = 0.0;

            for(int index : ransac.inliers){
                mean_error += reprojection_error(
                    ransac.H,
                    source_points[index],
                    destination_points[index]
                );
            }

            mean_error /= ransac.num_inliers;

            IMAGE_EDGE edge;
            edge.image_a = a;
            edge.image_b = b;
            edge.homography_a_to_b = ransac.H;
            edge.inliers = ransac.num_inliers;
            edge.mean_reprojection_error = mean_error;
            edge.matches = std::move(matches);
            edge.inlier_indices = std::move(ransac.inliers);

            graph.edges.push_back(std::move(edge));
        }
    }

    return graph;
}

std::vector<IMAGE_EDGE> maximum_spanning_tree(const IMAGE_GRAPH& graph){
    std::vector<IMAGE_EDGE> sorted = graph.edges;

    // more ransac inliers means the images have a stronger geometric relationship
    // if two edges have the same amount, prefer the one with less reprojection error
    std::sort(sorted.begin(), sorted.end(), [](const IMAGE_EDGE& a, const IMAGE_EDGE& b){
        if(a.inliers != b.inliers){
            return a.inliers > b.inliers;
        }

        return a.mean_reprojection_error < b.mean_reprojection_error;
    });

    DISJOINT_SET sets(graph.image_count);
    std::vector<IMAGE_EDGE> tree;

    if(graph.image_count > 0){
        tree.reserve(graph.image_count - 1);
    }

    // Kruskal normally picks the cheapest edges, here we sorted backwards because stronger matches are better
    for(const IMAGE_EDGE& edge : sorted){
        if(!sets.join(edge.image_a, edge.image_b)){
            continue;
        }

        tree.push_back(edge);

        if(tree.size() + 1 == graph.image_count){
            break;
        }
    }

    return tree;
}

std::size_t choose_reference_image(
    std::size_t image_count,
    const std::vector<IMAGE_EDGE>& tree
){
    if(image_count == 0){
        return 0;
    }

    std::vector<int> strength(image_count, 0);

    // use the image with the strongest set of direct relationships as our coordinate origin
    // this usually keeps the panorama from growing mostly to one side like an end image would
    for(const IMAGE_EDGE& edge : tree){
        strength[edge.image_a] += edge.inliers;
        strength[edge.image_b] += edge.inliers;
    }

    return static_cast<std::size_t>(
        std::distance(strength.begin(), std::max_element(strength.begin(), strength.end()))
    );
}

bool compute_image_transforms(
    std::size_t image_count,
    const std::vector<IMAGE_EDGE>& tree,
    std::size_t reference_image,
    std::vector<Eigen::Matrix3d>& transforms_to_reference
){
    if(image_count == 0 || reference_image >= image_count){
        return false;
    }

    struct GRAPH_STEP{
        std::size_t neighbor = 0;
        Eigen::Matrix3d neighbor_to_current = Eigen::Matrix3d::Identity();
    };

    std::vector<std::vector<GRAPH_STEP>> adjacency(image_count);

    for(const IMAGE_EDGE& edge : tree){
        // H maps A -> B
        // if we are standing on A, B needs H^-1 to get back into A's coordinate system
        adjacency[edge.image_a].push_back({
            edge.image_b,
            edge.homography_a_to_b.inverse()
        });

        // if we are standing on B, A already maps into B using H
        adjacency[edge.image_b].push_back({
            edge.image_a,
            edge.homography_a_to_b
        });
    }

    transforms_to_reference.assign(image_count, Eigen::Matrix3d::Identity());
    std::vector<bool> visited(image_count, false);
    std::queue<std::size_t> pending;

    visited[reference_image] = true;
    pending.push(reference_image);

    while(!pending.empty()){
        std::size_t current = pending.front();
        pending.pop();

        for(const GRAPH_STEP& step : adjacency[current]){
            if(visited[step.neighbor]){
                continue;
            }

            // neighbor -> current -> reference, matrix multiplication happens in that same right-to-left order
            transforms_to_reference[step.neighbor] =
                transforms_to_reference[current] * step.neighbor_to_current;

            visited[step.neighbor] = true;
            pending.push(step.neighbor);
        }
    }

    return std::all_of(visited.begin(), visited.end(), [](bool value){
        return value;
    });
}
