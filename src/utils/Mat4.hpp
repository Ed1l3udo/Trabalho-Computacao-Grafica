#pragma once
#include <cmath>
#include <stdexcept>
#include "Vec4.hpp"

struct Mat4 {
    double m[4][4];

    static Mat4 identity() {
        Mat4 A{};
        for (int r=0;r<4;r++) for(int c=0;c<4;c++) A.m[r][c] = (r==c)?1.0:0.0;
        return A;
    }

    static Mat4 translation(double tx, double ty, double tz) {
        Mat4 A = identity();
        A.m[0][3] = tx;
        A.m[1][3] = ty;
        A.m[2][3] = tz;
        return A;
    }

    static Mat4 scale(double sx, double sy, double sz) {
        Mat4 A = identity();
        A.m[0][0] = sx;
        A.m[1][1] = sy;
        A.m[2][2] = sz;
        return A;
    }

    static Mat4 rotateX(double rad) {
        Mat4 A = identity();
        double c = std::cos(rad), s = std::sin(rad);
        A.m[1][1] = c;  A.m[1][2] = -s;
        A.m[2][1] = s;  A.m[2][2] = c;
        return A;
    }

    static Mat4 rotateY(double rad) {
        Mat4 A = identity();
        double c = std::cos(rad), s = std::sin(rad);
        A.m[0][0] = c;  A.m[0][2] = s;
        A.m[2][0] = -s; A.m[2][2] = c;
        return A;
    }

    static Mat4 rotateZ(double rad) {
        Mat4 A = identity();
        double c = std::cos(rad), s = std::sin(rad);
        A.m[0][0] = c;  A.m[0][1] = -s;
        A.m[1][0] = s;  A.m[1][1] = c;
        return A;
    }

    // Rotação em eixo arbitrário (Rodrigues). axis deve ter w=0.
    static Mat4 rotateAxisAngle(const Vec4& axisRaw, double rad) {
        Vec4 axis = axisRaw.normalize();
        double x = axis.x, y = axis.y, z = axis.z;
        double c = std::cos(rad), s = std::sin(rad), t = 1.0 - c;

        Mat4 A = identity();
        A.m[0][0] = t*x*x + c;
        A.m[0][1] = t*x*y - s*z;
        A.m[0][2] = t*x*z + s*y;

        A.m[1][0] = t*x*y + s*z;
        A.m[1][1] = t*y*y + c;
        A.m[1][2] = t*y*z - s*x;

        A.m[2][0] = t*x*z - s*y;
        A.m[2][1] = t*y*z + s*x;
        A.m[2][2] = t*z*z + c;

        return A;
    }

    // Cisalhamento (shear): cada termo diz quanto um eixo “puxa” outro.
    // x' = x + shXY*y + shXZ*z,  y' = y + shYX*x + shYZ*z,  z' = z + shZX*x + shZY*y
    static Mat4 shear(double shXY, double shXZ, double shYX, double shYZ, double shZX, double shZY) {
        Mat4 A = identity();
        A.m[0][1] = shXY; A.m[0][2] = shXZ;
        A.m[1][0] = shYX; A.m[1][2] = shYZ;
        A.m[2][0] = shZX; A.m[2][1] = shZY;
        return A;
    }

    // Espelho em relação a um plano arbitrário definido por ponto P0 e normal n (w=0).
    static Mat4 mirrorPlane(const Vec4& P0, const Vec4& nRaw) {
        Vec4 n = nRaw.normalize();
        double nx=n.x, ny=n.y, nz=n.z;

        // R = I - 2 n n^T
        Mat4 A = identity();
        A.m[0][0] = 1 - 2*nx*nx; A.m[0][1] = -2*nx*ny;   A.m[0][2] = -2*nx*nz;
        A.m[1][0] = -2*ny*nx;    A.m[1][1] = 1 - 2*ny*ny; A.m[1][2] = -2*ny*nz;
        A.m[2][0] = -2*nz*nx;    A.m[2][1] = -2*nz*ny;   A.m[2][2] = 1 - 2*nz*nz;

        // t = P0 - R*P0
        Vec4 RP0 = A.mulPoint(P0);
        A.m[0][3] = P0.x - RP0.x;
        A.m[1][3] = P0.y - RP0.y;
        A.m[2][3] = P0.z - RP0.z;

        return A;
    }

    Mat4 operator*(const Mat4& B) const {
        Mat4 C{};
        for (int r=0;r<4;r++) {
            for (int c=0;c<4;c++) {
                double s = 0.0;
                for (int k=0;k<4;k++) s += m[r][k]*B.m[k][c];
                C.m[r][c] = s;
            }
        }
        return C;
    }

    Vec4 mulPoint(const Vec4& p) const {
        double x = m[0][0]*p.x + m[0][1]*p.y + m[0][2]*p.z + m[0][3]*p.w;
        double y = m[1][0]*p.x + m[1][1]*p.y + m[1][2]*p.z + m[1][3]*p.w;
        double z = m[2][0]*p.x + m[2][1]*p.y + m[2][2]*p.z + m[2][3]*p.w;
        double w = m[3][0]*p.x + m[3][1]*p.y + m[3][2]*p.z + m[3][3]*p.w;
        return Vec4(x,y,z,w);
    }

    Vec4 mulVector(const Vec4& v) const {
        // vetor: w=0
        Vec4 vv(v.x, v.y, v.z, 0);
        double x = m[0][0]*vv.x + m[0][1]*vv.y + m[0][2]*vv.z;
        double y = m[1][0]*vv.x + m[1][1]*vv.y + m[1][2]*vv.z;
        double z = m[2][0]*vv.x + m[2][1]*vv.y + m[2][2]*vv.z;
        return Vec4(x,y,z,0);
    }

    Mat4 transpose() const {
        Mat4 T{};
        for (int r=0;r<4;r++) for(int c=0;c<4;c++) T.m[r][c] = m[c][r];
        return T;
    }

    Mat4 inverse() const {
        // Gauss-Jordan em matriz aumentada [A | I]
        double a[4][8]{};
        for (int r=0;r<4;r++) {
            for (int c=0;c<4;c++) a[r][c] = m[r][c];
            for (int c=0;c<4;c++) a[r][c+4] = (r==c)?1.0:0.0;
        }

        for (int col=0; col<4; col++) {
            // pivô
            int piv = col;
            for (int r=col+1;r<4;r++)
                if (std::abs(a[r][col]) > std::abs(a[piv][col])) piv = r;
            if (std::abs(a[piv][col]) < 1e-12) throw std::runtime_error("Mat4 inverse: singular");

            if (piv != col) {
                for (int c=0;c<8;c++) std::swap(a[piv][c], a[col][c]);
            }

            double div = a[col][col];
            for (int c=0;c<8;c++) a[col][c] /= div;

            for (int r=0;r<4;r++) if (r!=col) {
                double f = a[r][col];
                for (int c=0;c<8;c++) a[r][c] -= f * a[col][c];
            }
        }

        Mat4 inv{};
        for (int r=0;r<4;r++) for(int c=0;c<4;c++) inv.m[r][c] = a[r][c+4];
        return inv;
    }
};
