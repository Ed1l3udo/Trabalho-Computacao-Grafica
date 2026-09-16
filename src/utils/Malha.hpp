#pragma once
#include <vector>
#include <array>
#include <string>
#include <limits>
#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "Objeto.hpp"
#include "Vec4.hpp"
#include "Color.hpp"
#include "Luz.hpp"
#include "Colisao.hpp"
#include "Transform.hpp"
#include "RayTriangle.hpp"
#include "OBJLoader.hpp"

// Malha triangulada
class Malha : public Objeto {
public:
    std::vector<Vec4> vertices;                 // (x,y,z,w=1) no espaço local
    std::vector<std::array<int,3>> tris;        // índices 0-based

    // “Materiais” no seu padrão
    Vec4 Ke, Kd, Ka;
    double m;

    // estado do último hit (por pixel)
    mutable int lastTri = -1;
    mutable Vec4 lastNormalLocal = Vec4(0,1,0,0);

    Malha(
        const std::string& objPath,
        Vec4 Ke = {1,1,1,0},
        Vec4 Kd = {1,1,1,0},
        Vec4 Ka = {0.1,0.1,0.1,0},
        double m = 50.0
    ) : Ke(Ke), Kd(Kd), Ka(Ka), m(m)
    {
        std::string err;
        if (!loadOBJ_positions_faces(objPath, vertices, tris, &err)) {
            throw std::runtime_error(err);
        }
    }

    bool intersectLocal(const Vec4& origemL, const Vec4& dirL,
                        Vec4& intersectionL, double& tL, Colisao& tipo) const override
    {
        const double EPS = 1e-6;
        double bestT = std::numeric_limits<double>::infinity();
        int bestTri = -1;

        double bestU=0, bestV=0;

        for (int i=0; i<(int)tris.size(); i++) {
            auto tri = tris[i];
            const Vec4& v0 = vertices[tri[0]];
            const Vec4& v1 = vertices[tri[1]];
            const Vec4& v2 = vertices[tri[2]];

            double t,u,v;
            if (intersect_ray_triangle_mt(origemL, dirL, v0, v1, v2, t, u, v)) {
                if (t > EPS && t < bestT) {
                    bestT = t;
                    bestTri = i;
                    bestU = u; bestV = v;
                }
            }
        }

        if (bestTri < 0) return false;

        tL = bestT;
        intersectionL = origemL + dirL * tL;
        tipo = Colisao::Corpo; // ou Nenhuma se você preferir

        // normal de face (local)
        auto tri = tris[bestTri];
        Vec4 e1 = vertices[tri[1]] - vertices[tri[0]];
        Vec4 e2 = vertices[tri[2]] - vertices[tri[0]];
        Vec4 nL = normalize(e1.cross(e2));   // pode manter assim

        // ✅ Garante que a normal aponte "para fora" em relação ao raio (para a câmera)
        if (nL.dot(dirL) > 0.0) nL = nL * -1.0;

        // guarda para o shading
        lastNormalLocal = nL;   
        lastTri = bestTri;

        (void)bestU; (void)bestV; // futuro: usar para interpolar UV/normais
        return true;
    }

    Color calculaCor(const Vec4& origemW, const Vec4& intersectionW,
                     const Luz& luz, const Luz& luzAmb,
                     const Colisao& /*tipo*/, bool isInShadow) const override
    {
        // normal no mundo via normal matrix
        Vec4 n = normalToWorld(lastNormalLocal);

        Vec4 Ia = hadamard(luzAmb.intensidade, Ka);

        Vec4 I = Ia;
        if (!isInShadow) {
            const double fatt = luz.factorAt(intersectionW);
            if (std::isfinite(fatt) && fatt > 0.0) {
                Vec4 l = luz.Lvec(intersectionW);
                Vec4 v = normalize(origemW - intersectionW);

                double cosNL = std::max(0.0, n.dot(l));

                Vec4 r = normalize(n * (2.0 * cosNL) - l);
                double cosVR = std::max(0.0, v.dot(r));
                double specpow = (cosVR > 0.0) ? std::pow(cosVR, m) : 0.0;

                Vec4 I_luz = luz.intensidade * fatt;
                Vec4 Id = hadamard(I_luz, Kd) * cosNL;
                Vec4 Is = hadamard(I_luz, Ke) * specpow; // seu Ke = “Ks”
                I = Ia + Id + Is;
            }
        }

        auto sat01 = [](double x){ return std::max(0.0, std::min(1.0, x)); };
        int R = (int)(sat01(I.x) * 255.0);
        int G = (int)(sat01(I.y) * 255.0);
        int B = (int)(sat01(I.z) * 255.0);

        return Color(R,G,B);
    }
};
