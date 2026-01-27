#pragma once
#include <cmath>
#include "Vec4.hpp"

static inline bool intersect_ray_plane(
    const Vec4& origin, const Vec4& dir,
    const Vec4& P0, const Vec4& n,
    Vec4& intersection, double& t)
{
    const double EPS = 1e-6;
    double denom = n.dot(dir);
    if (std::abs(denom) < EPS) return false;

    Vec4 w = origin - P0;
    t = -(n.dot(w)) / denom;
    if (t <= EPS) return false;

    intersection = origin + dir * t;
    return true;
}
