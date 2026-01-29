#pragma once
#include <cmath>
#include "Vec4.hpp"
#include "Mat4.hpp"
#include "Transform.hpp" // normalize()

struct RTQuat {
    // q = w + xi + yj + zk
    double w, x, y, z;

    RTQuat(double w=1, double x=0, double y=0, double z=0) : w(w), x(x), y(y), z(z) {}

    static RTQuat fromAxisAngle(const Vec4& axisRaw, double angleRad) {
        Vec4 axis = normalize(Vec4(axisRaw.x, axisRaw.y, axisRaw.z, 0));
        double half = angleRad * 0.5;
        double s = std::sin(half);
        return RTQuat(std::cos(half), axis.x*s, axis.y*s, axis.z*s);
    }

    RTQuat normalized() const {
        double n = std::sqrt(w*w + x*x + y*y + z*z);
        if (n <= 1e-12) return RTQuat();
        return RTQuat(w/n, x/n, y/n, z/n);
    }

    RTQuat operator*(const RTQuat& b) const {
        return RTQuat(
            w*b.w - x*b.x - y*b.y - z*b.z,
            w*b.x + x*b.w + y*b.z - z*b.y,
            w*b.y - x*b.z + y*b.w + z*b.x,
            w*b.z + x*b.y - y*b.x + z*b.w
        );
    }

    Mat4 toMat4() const {
        RTQuat q = normalized();
        double ww=q.w, xx=q.x, yy=q.y, zz=q.z;

        Mat4 M = Mat4::identity();

        double xx2 = 2*xx*xx, yy2 = 2*yy*yy, zz2 = 2*zz*zz;
        double xy2 = 2*xx*yy, xz2 = 2*xx*zz, yz2 = 2*yy*zz;
        double wx2 = 2*ww*xx, wy2 = 2*ww*yy, wz2 = 2*ww*zz;

        M.m[0][0] = 1 - yy2 - zz2;
        M.m[0][1] = xy2 - wz2;
        M.m[0][2] = xz2 + wy2;

        M.m[1][0] = xy2 + wz2;
        M.m[1][1] = 1 - xx2 - zz2;
        M.m[1][2] = yz2 - wx2;

        M.m[2][0] = xz2 - wy2;
        M.m[2][1] = yz2 + wx2;
        M.m[2][2] = 1 - xx2 - yy2;

        return M;
    }
};
