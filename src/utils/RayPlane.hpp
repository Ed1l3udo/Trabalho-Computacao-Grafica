#pragma once
#include <cmath>
#include "Vec4.hpp"

//Plano

bool intersect_ray_plane(const Vec4& origin, const Vec4& dir, const Vec4& P0, const Vec4& n, Vec4& intersection, double& t)
{
    double denom = n.dot(dir);
    if(denom == 0)
    {
        return false;
    }
    Vec4 w = origin - P0;
    t = -(n.dot(w))/denom;
    if(t < 0)
    {
        return false;
    }

    intersection = origin + dir*t;

    return true;
}

