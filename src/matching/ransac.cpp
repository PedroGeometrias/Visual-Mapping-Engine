#include "matching/ransac.hpp"

#include "geometry/homography.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <set>

// it picks count unique rando indices from 0 -> num_matches
static std::vector<int> get_random_indices(
    int num_matches,
    int count,
    // the random in the function name comes from this guy
    std::mt19937& generator
){
    std::vector<int> indices;
    std::set<int> used;

    if(count > num_matches){
        return indices;
    }

    std::uniform_int_distribution<int> distribution(0, num_matches - 1);

    // ransac needs 4 different matches, repeating an index would mean that we really sampled less than 4
    while(static_cast<int>(indices.size()) < count){
        int index = distribution(generator);

        if(used.find(index) != used.end()){
            continue;
        }

        used.insert(index);
        indices.push_back(index);
    }

    return indices;
}

RANSAC_RESULT ransac_homography(
    // matched 2d points in image A and img B
    const std::vector<Eigen::Vector2d>& src_points,
    const std::vector<Eigen::Vector2d>& dst_points,
    // how much reprojection error is still considered an inlier
    double inlier_threshold,
    // hard cap on the ransac iterations
    // target probability that at least one sample is all-inlier
    int max_iterations,
    double confidence
){
    RANSAC_RESULT best;

    if(src_points.size() != dst_points.size() || src_points.size() < 4){
        return best;
    }

    const int num_matches = static_cast<int>(src_points.size());

    std::random_device random_device;
    std::mt19937 generator(random_device());

    int iteration = 0;
    int iteration_limit = max_iterations;

    while(iteration < iteration_limit){
        // randomly grab the minimum 4 correspondences needed to build a homography
        std::vector<int> sample_indices = get_random_indices(num_matches, 4, generator);
        std::vector<Eigen::Vector2d> sample_src(4);
        std::vector<Eigen::Vector2d> sample_dst(4);

        for(int i = 0; i < 4; i++){
            sample_src[i] = src_points[sample_indices[i]];
            sample_dst[i] = dst_points[sample_indices[i]];
        }

        // 4 points on the same line cannot define a proper projective transformation
        if(are_collinear(sample_src) || are_collinear(sample_dst)){
            iteration++;
            continue;
        }

        // pretend these 4 matches are correct and build the homography from them
        Eigen::Matrix3d H = compute_homography_dlt(sample_src, sample_dst);

        std::vector<int> inliers;

        // now test that guess against every match we actually have
        for(int i = 0; i < num_matches; i++){
            double error = reprojection_error(H, src_points[i], dst_points[i]);

            if(error < inlier_threshold){
                inliers.push_back(i);
            }
        }

        // the model that explains the largest amount of matches is our current best guess
        if(static_cast<int>(inliers.size()) > best.num_inliers){
            best.H = H;
            best.inliers = inliers;
            best.num_inliers = static_cast<int>(inliers.size());

            // if every match agrees with this homography there is nothing left for ransac to search for
            if(best.num_inliers == num_matches){
                break;
            }

            // once we know roughly how many matches are good we can estimate how many attempts we really need
            // w^4 is the chance that all 4 randomly selected matches are inliers in one iteration
            const double inlier_ratio = static_cast<double>(best.num_inliers) / num_matches;
            const double miss_probability = 1.0 - std::pow(inlier_ratio, 4.0);

            if(miss_probability > 0.0 && miss_probability < 1.0){
                const double new_max =
                    std::log(1.0 - confidence) /
                    std::log(miss_probability);

                const int adaptive_limit = static_cast<int>(std::ceil(new_max));
                iteration_limit = std::min(
                    iteration_limit,
                    std::max(iteration + 1, adaptive_limit)
                );
            }
        }

        iteration++;
    }

    // the random sample only found the consensus, now use every inlier to get a better final homography
    if(best.num_inliers >= 4){
        std::vector<Eigen::Vector2d> inlier_src;
        std::vector<Eigen::Vector2d> inlier_dst;
        inlier_src.reserve(best.inliers.size());
        inlier_dst.reserve(best.inliers.size());

        for(int index : best.inliers){
            inlier_src.push_back(src_points[index]);
            inlier_dst.push_back(dst_points[index]);
        }

        best.H = compute_homography_dlt(inlier_src, inlier_dst);
    }

    return best;
}
