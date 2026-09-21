#include "geometry/homography.hpp"

#include <cassert>
#include <cmath>
#include <limits>

#include <Eigen/SVD>

static Eigen::Matrix3d normalization_transform(
    const std::vector<Eigen::Vector2d>& points
){
    Eigen::Vector2d center(0.0, 0.0);

    for(const Eigen::Vector2d& point : points){
        center += point;
    }

    center /= static_cast<double>(points.size());

    double mean_distance = 0.0;

    for(const Eigen::Vector2d& point : points){
        mean_distance += (point - center).norm();
    }

    mean_distance /= static_cast<double>(points.size());

    // DLT behaves better when our coordinates live around the origin and have a similar scale
    // sqrt(2) is the conventional target average distance for 2D normalized DLT
    double scale = mean_distance > 1e-12
        ? std::sqrt(2.0) / mean_distance
        : 1.0;

    Eigen::Matrix3d transform = Eigen::Matrix3d::Identity();
    transform(0, 0) = scale;
    transform(1, 1) = scale;
    transform(0, 2) = -scale * center.x();
    transform(1, 2) = -scale * center.y();

    return transform;
}

static std::vector<Eigen::Vector2d> normalize_points(
    const std::vector<Eigen::Vector2d>& points,
    const Eigen::Matrix3d& transform
){
    std::vector<Eigen::Vector2d> normalized;
    normalized.reserve(points.size());

    for(const Eigen::Vector2d& point : points){
        Eigen::Vector3d homogeneous(point.x(), point.y(), 1.0);
        Eigen::Vector3d transformed = transform * homogeneous;
        normalized.emplace_back(transformed.x(), transformed.y());
    }

    return normalized;
}

// a homography has 9 values, but because it is homogeneous the matrix is only defined up to scale
// each point match gives us 2 equations, so with 4 matches we have enough information to solve it
Eigen::Matrix3d compute_homography_dlt(
    const std::vector<Eigen::Vector2d>& src_points,
    const std::vector<Eigen::Vector2d>& dst_points
){
    assert(src_points.size() == dst_points.size() && src_points.size() >= 4);

    Eigen::Matrix3d source_normalization = normalization_transform(src_points);
    Eigen::Matrix3d destination_normalization = normalization_transform(dst_points);

    std::vector<Eigen::Vector2d> normalized_source =
        normalize_points(src_points, source_normalization);
    std::vector<Eigen::Vector2d> normalized_destination =
        normalize_points(dst_points, destination_normalization);

    // 2 equations per match and 9 unknown homography values
    Eigen::MatrixXd A(2 * src_points.size(), 9);
    A.setZero();

    for(std::size_t i = 0; i < normalized_source.size(); i++){
        const double x = normalized_source[i].x();
        const double y = normalized_source[i].y();
        const double u = normalized_destination[i].x();
        const double v = normalized_destination[i].y();

        // these are the two rows that come from x,y -> u,v
        // we are building A so later we can solve A * h = 0
        A(2 * i, 0) = -x;
        A(2 * i, 1) = -y;
        A(2 * i, 2) = -1.0;
        A(2 * i, 6) = u * x;
        A(2 * i, 7) = u * y;
        A(2 * i, 8) = u;

        A(2 * i + 1, 3) = -x;
        A(2 * i + 1, 4) = -y;
        A(2 * i + 1, 5) = -1.0;
        A(2 * i + 1, 6) = v * x;
        A(2 * i + 1, 7) = v * y;
        A(2 * i + 1, 8) = v;
    }

    // SVD gives us the vector that best satisfies A * h = 0
    // the last column of V is the right singular vector with the smallest singular value
    Eigen::JacobiSVD<Eigen::MatrixXd> svd(A, Eigen::ComputeFullV);
    Eigen::VectorXd h = svd.matrixV().col(8);

    // turn the 9 values back into our 3x3 homography matrix
    Eigen::Matrix3d normalized_H;
    normalized_H << h(0), h(1), h(2),
                    h(3), h(4), h(5),
                    h(6), h(7), h(8);

    // the solver worked in normalized coordinates, undo those transforms so H works on real image pixels again
    Eigen::Matrix3d H =
        destination_normalization.inverse() * normalized_H * source_normalization;

    // homographies are scale invariant, so H and 2H mean the same thing
    // making the last value 1 gives us a nicer and more predictable matrix to inspect
    if(std::abs(H(2, 2)) > 1e-12){
        H /= H(2, 2);
    }

    return H;
}

Eigen::Vector2d apply_homography(
    const Eigen::Matrix3d& H,
    const Eigen::Vector2d& point
){
    // homographies work in homogeneous coordinates, the extra 1 lets translation live inside the matrix
    Eigen::Vector3d homogeneous(point.x(), point.y(), 1.0);
    Eigen::Vector3d projected = H * homogeneous;

    // after the matrix multiplication we divide by z to get back to normal image coordinates
    // if z is basically zero the point went somewhere invalid, return infinity so ransac rejects it
    if(std::abs(projected.z()) < 1e-12){
        const double infinity = std::numeric_limits<double>::infinity();
        return Eigen::Vector2d(infinity, infinity);
    }

    return Eigen::Vector2d(
        projected.x() / projected.z(),
        projected.y() / projected.z()
    );
}

double reprojection_error(
    const Eigen::Matrix3d& H,
    const Eigen::Vector2d& source,
    const Eigen::Vector2d& destination
){
    // project the source through H, then measure how far it landed from the match we expected
    Eigen::Vector2d projected = apply_homography(H, source);
    return (projected - destination).norm();
}

bool are_collinear(const std::vector<Eigen::Vector2d>& points){
    if(points.size() < 4){
        return true;
    }

    // a homography sample becomes degenerate if any 3 of our 4 points are sitting on one line
    // the determinant below is twice the signed area of the triangle, zero means no triangle at all
    for(std::size_t a = 0; a < points.size() - 2; a++){
        for(std::size_t b = a + 1; b < points.size() - 1; b++){
            for(std::size_t c = b + 1; c < points.size(); c++){
                const double det =
                    points[a].x() * (points[b].y() - points[c].y()) -
                    points[b].x() * (points[a].y() - points[c].y()) +
                    points[c].x() * (points[a].y() - points[b].y());

                if(std::abs(det) < 1e-6){
                    return true;
                }
            }
        }
    }

    return false;
}
