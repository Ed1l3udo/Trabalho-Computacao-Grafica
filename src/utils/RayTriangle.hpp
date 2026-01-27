#pragma once
#include <cmath>
#include "Vec4.hpp"
#include "Transform.hpp" // precisa do cross(a,b)

// Möller–Trumbore (sem backface culling)
static inline bool intersect_ray_triangle_mt(
    const Vec4& orig,
    const Vec4& dir,
    const Vec4& v0,
    const Vec4& v1,
    const Vec4& v2,
    double& t,
    double& u,
    double& v
) {
    const double EPS = 1e-9;

    Vec4 e1 = v1 - v0;
    Vec4 e2 = v2 - v0;

    Vec4 pvec = dir.cross(e2);
    double det = e1.dot(pvec);

    if (std::abs(det) < EPS) return false; // paralelo

    double invDet = 1.0 / det;

    Vec4 tvec = orig - v0;
    u = tvec.dot(pvec) * invDet;
    if (u < 0.0 || u > 1.0) return false;

    Vec4 qvec = tvec.cross(e1);
    v = dir.dot(qvec) * invDet;
    if (v < 0.0 || (u + v) > 1.0) return false;

    t = e2.dot(qvec) * invDet;
    if (t <= EPS) return false;

    return true;
}
